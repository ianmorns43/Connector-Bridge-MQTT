 import groovy.json.JsonSlurper
 import groovy.json.JsonOutput

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
        capability "Configuration"
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
    //TODO set hubStatus based in lwt
    def message = interfaces.mqtt.parseMessage(description)
    logTrace(message)

    def topic = message.topic
    if(topic.endsWith("/lwt"))
    {
        sendEvent(name: "hubStatus", value: message.payload)
        return
    }

    def slurper = new JsonSlurper()
    def attributes = slurper.parseText(message.payload)
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

    if(topic.endsWith("status"))
    {
        unschedule(refreshTimeout)
        setShadeBasedOnPosition(attributes.position)
    }

    if(topic.endsWith("commandReceived"))
    {
        sendEvent(name: "windowShade", value: state.lastMovementDirection);
    }

    if(topic.endsWith("moveComplete"))
    {
        unschedule(movementTimeout)
        setShadeBasedOnPosition(attributes.position)
    }
}

private updateAttribute(Map attributes, String attributeName)
{
    if(attributes.containsKey(attributeName))
    {
        sendEvent(name: attributeName, value: attributes[attributeName])
    }
}

def setShadeBasedOnPosition(position)
{
    def shade = position == 0 ? "closed" : (position == 100 ? "open" : "partially open")
    sendEvent(name: "windowShade", value: shade);
}

def setPosition(position)
{
    logTrace("SetPosition: ${position}");
    state.lastMovementDirection = position > device.currentValue("position") ? "opening" : "closing"
    publishWithRetry([command:"moveShade", includeKey:true, parameters:[position:position]], [position:position])
}

def open()
{
    logTrace("Open")
    state.lastMovementDirection = "opening"
    publishWithRetry([command:"openShade", includeKey:true], [windowShade:"open"])
}

def close()
{
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
    publish([command:"stopShade", includeKey:true])
    runIn(1, refresh);
}

def refresh()
{
    def retryInterval = 10
    publish(refreshAction())
    runIn(retryInterval, refreshTimeout, [data: [lastRetryInterval:retryInterval]])
}

def refreshWithoutRetry()
{
    publish(refreshAction())
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
    publish(action)
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

    publish(refreshAction())
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

    publish(action)
    runIn(retryInterval, movementTimeout, [data: data])
}

def publish(Map action)
{
    if(!getMac())
    {
        return
    }
    String command = action.command
    Boolean includeKey = action.includeKey
    Map parameters = action.parameters

    reconectIfNessecary()

    def details = parent.getHubDetails()
    def payload = [command:command, mac:getMac()]

    if(includeKey)
    {
        payload["key"] = details.hubKey
    }

    if(parameters)
    {
        payload << parameters
    }

    def topic = "${details.hubTopic}/command"
    //logTrace("Publish: ${topic}, ${payload}")
    def payloadJson = JsonOutput.toJson(payload)

    
    logTrace("Publish: ${topic}, ${payloadJson}")
    interfaces.mqtt.publish(topic, payloadJson)    
}

def installed()
{
    initialize();
}

def uninstalled()
{
    logTrace("Uninstalling")
    if(interfaces.mqtt.isConnected())
    {
        interfaces.mqtt.disconnect()
    }
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

    if(settings.mac && settings.mac != device.data.lastSubscribedMac)
    {
        configure()
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

def subscritionTopics(mac)
{
    def details = parent.getHubDetails()
    def hubTopic = details.hubTopic

    topics = []
    topics << "${details.hubTopic}/${mac}/+"
    topics << "${details.hubTopic}/lwt"
    return topics
}


public setMac(mac)
{
    device.updateSetting("mac", mac)
    if(mac != device.data.lastSubscribedMac)
    {
        configure()
    }
}

public getMac()
{
    return settings.mac
}

public configure()
{
    logTrace("Broker details updated")
    if(interfaces.mqtt.isConnected())
    {
        interfaces.mqtt.disconnect()
    }
    refresh()
}

public brokerStatusChanged(evt)
{
    if(evt.value == "offline")
    {
        sendEvent(name: "hubStatus", value: "unknown")
    }
    else
    {
        reconectIfNessecary()
    }
}

def reconectIfNessecary()
{
    if(!interfaces.mqtt.isConnected())
    {
        logTrace("attempting to reconnect")
        connectAndSubscribe()
    }
}

def connectAndSubscribe()
{
    def connectionDetails = parent.getHubDetails()
    def mac = getMac()
    if(!interfaces.mqtt.isConnected() && mac)
    {
        try
        {
            interfaces.mqtt.connect(connectionDetails.path, device.deviceNetworkId, connectionDetails.username, connectionDetails.password)
            subscritionTopics(mac).each
            { 
                topic -> interfaces.mqtt.subscribe(topic)
                logTrace("Subscribed to: ${topic}")
            }

            device.updateDataValue("lastSubscribedMac", mac)
        }
        catch(exception)
        {
            logTrace("Connection error: " + exception)
            sendEvent(name: "hubStatus", value: "unknown")
        }
    }    
}

def mqttClientStatus(String message)
{
    logTrace("mqttClientStatus $message")
    
    if(message.indexOf("Connection lost") >= 0 || message.indexOf("Client is not connected") >= 0)
    {
        logTrace("Lost connection so hubStatus is nolonger known")
        sendEvent(name: "hubStatus", value: "unknown")
    }
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

