 /**
 *  Dooya Bidirectional Blind
 *
 *  Copyright 2020 Ian Morns
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *  in compliance with the License. You may obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *  for the specific language governing permissions and limitations under the License.
 *
 */
metadata {
	definition (name: "Dooya Bidirectional Blind", namespace: "ianmorns_connector", author: "Ian Morns") {
		capability "WindowShade"        
        capability "Switch"
        capability "Refresh"
        capability "Battery"
        capability "SignalStrength"
        capability "Actuator"

        attribute "hubStatus", "ENUM", ["online","offline","unknown"]
	}
   
    section() 
    {
        input "refreshInterval", "number", title: "Refresh polling interval (min)", required: true, defaultValue: 20
        input "enableLogging", "bool", title: "Enable debug logging for 30 minutes", multiple: false, defaultValue: true
        input "mac", "text", title: "Device Mac Address (changing this value could unlink device from hub)", multiple: false
    }
}

// parse events into attributes
def parse(String description)
{
}

def parse(Map message)
{
    //parent.forwardMessage([deviceMac:deviceMac, messageType:messageType, payload:payload])
    //TODO set hubStatus based in lwt

    logTrace("At device: ${message}")
    if(message.messageType == "lwt")
    {
        sendEvent(name: "hubStatus", value: message.payload)
        return
    }

    sendEvent(name: "hubStatus", value: "online")

    def attributes = message.payload
    logTrace(attributes)

    if(attributes.containsKey("shadeType"))
    {
        device.setName(attributes.shadeType)
    }

    if(attributes.containsKey("battery"))
    {
        sendEvent(name: "battery", value: attributes.battery / 100, unit: "V")
    }

    if(attributes.containsKey("rssi"))
    {
        sendEvent(name: "rssi", value: attributes.rssi)
    }

    if(attributes.containsKey("position"))
    {
        sendEvent(name:"position", value: attributes.position)
    }

    if(message.messageType == "status")
    {
        unschedule(refreshTimeout)
        setShadeBasedOnPosition(attributes.position)
    }

    if(message.messageType == "moving")
    {
        sendEvent(name: "windowShade", value: state.lastMovementDirection);
    }

    if(message.messageType == "moved")
    {
        unschedule(movementTimeout)
        setShadeBasedOnPosition(attributes.position)
    }
}

def setShadeBasedOnPosition(position)
{
    def shade = position == 0 ? "closed" : (position == 100 ? "open" : "partially open")
    sendEvent(name: "windowShade", value: shade);
}

def isActive()
{
    def active = device.currentValue("switch")

    //Make on the default value
    if(active == null)
    {
        active = "on"
        sendEvent(name: "switch", value: "on")
    }

    return active != "off"
}

def on()
{
    sendEvent(name: "switch", value: "on")
}

def off()
{
    sendEvent(name: "switch", value: "off")
}

def setPosition(position)
{
    if(!isActive())
    {
        return
    }

    logTrace("SetPosition: ${position}");
    state.lastMovementDirection = position > device.currentValue("position") ? "opening" : "closing"
    publishWithRetry([command:"moveShade", includeKey:true, parameters:[position:position]], [position:position])
}

def open()
{
    if(!isActive())
    {
        return
    }

    logTrace("Open")
    state.lastMovementDirection = "opening"
    publishWithRetry([command:"openShade", includeKey:true], [windowShade:"open"])
}

def close()
{
    if(!isActive())
    {
        return
    }

    logTrace("Close")
    state.lastMovementDirection = "closing"
    publishWithRetry([command:"closeShade", includeKey:true], [windowShade:"closed"])
}

def startPositionChange(direction)
{
    "${direction}"()
}

def stopPositionChange()
{
    state.lastMovementDirection = "partially open"
    publishCommand([command:"stopShade", includeKey:true])
    runIn(1, refresh);
}

def refresh()
{
    def retryInterval = 10
    publishCommand(refreshAction())
    runIn(retryInterval, refreshTimeout, [data: [lastRetryInterval:retryInterval]])
}

def refreshWithoutRetry()
{
    publishCommand(refreshAction())
}

def refreshAction()
{
    return [command:"getStatus", includeKey:false]    
}

/*
dooya_connector_hub/command                     {command:"updateDeviceList"}
                                                {command:"moveShade", mac:"<deviceMac>", key:"<hub-secret-key>", position:<newPosition>}
                                                {command:"openShade", mac:"<deviceMac>", key:"<hub-secret-key>"}
                                                {command:"closeShade", mac:"<deviceMac>", key:"<hub-secret-key>"}
                                                {command:"stopShade", mac:"<deviceMac>", key:"<hub-secret-key>"}
                                                {command:"updateDevce", mac:"<deviceMac>"}
                                                */

def publishWithRetry(Map action, Map expected)
{
    def retryInterval = 120
    def data =[data: [action:action, nexAction:"refresh", lastRetryInterval:retryInterval, expected:expected]]
    publishCommand(action)
    runIn(retryInterval, movementTimeout, data)
}

def refreshTimeout(data)
{
    logTrace("Refresh Timeout: ${data}")
    Integer maximumRetryInterval = 600

    def retryInterval = (Integer) (data.lastRetryInterval * 1.5)
    if(retryInterval >= maximumRetryInterval)
    {
        logTrace("Just trying to refresh, may as well let the regular ping take over from here")
        return
    }
    
    data.lastRetryInterval = retryInterval;

    publishCommand(refreshAction())
    runIn(retryInterval, refreshTimeout, [data: data])
}

def movementTimeout(data)
{
    logTrace("Timeout: ${data}")
    Integer maximumRetryInterval = 600

    def expectedShade = data.expected?.windowShade
    def expectedPositon = data.expected?.position
    if( (expectedShade && expectedShade == device.currentValue("windowShade")) || (expectedPositon != null && expectedPositon == device.currentValue("position")))
    {
        logTrace("Blind is in expected state - stop retry")
        return
    }

    def nextAction = data.nextAction
    def retryInterval = 0
    def action = null

    if(nextAction == "action")
    {
        //Gradually make the retry interval longer until it reaches a maximum
        retryInterval = Math.min((Integer) (data.lastRetryInterval * 1.5), maximumRetryInterval)
        data.lastRetryInterval = retryInterval;

        data.nextAction = retryInterval < maximumRetryInterval ? "refresh" : "action"
        action = data.action
    }
    else //If we tried to move the blind, check to see if the blind has actually moved but we missed the notification before trying to movve it again
    {
        retryInterval = 5
        data.nextAction = "action"
        action = refreshAction()

    }

    publishCommand(action)
    runIn(retryInterval, movementTimeout, [data: data])
}

def publishCommand(Map action)
{
    if(!getMac())
    {
        return
    }
    String command = action.command
    Boolean includeKey = action.includeKey
    Map parameters = action.parameters

    def payload = [command:command, mac:getMac()]

    if(parameters)
    {
        payload << parameters
    }

    parent.publishCommand(payload, includeKey)    
}

def installed()
{
    initialize()
    refresh()
}

def uninstalled()
{
    logTrace("Uninstalling")
}

def updated()
{
    unschedule()
    initialize();
}

def initialize()
{
    if(settings.enableLogging)
    {
        runIn(1800, "disableLogging");
    }

    startPing()
}

def startPing()
{
    def refreshInterval = 20

    def second = Math.round(Math.random() * 59);
    def minute = Math.round(Math.random() * (refreshInterval - 1));

    schedule("${second} ${minute}/${refreshInterval} * ? * * *", "refreshWithoutRetry");
}

public getMac()
{
    return settings.mac
}

private disableLogging()
{
    device.updateSetting("enableLogging", false);
}

private logTrace(msg)
{
    if(settings.enableLogging)
    {
        log.trace msg
    }
}

