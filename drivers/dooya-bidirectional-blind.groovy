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
	definition (name: "Dooya Bidirectional Blind", namespace: "ianmorns_rfremote", author: "Ian Morns") {
		capability "WindowShade"
        capability "Configuration"
        capability "Refresh"
        capability "Battery"
        capability "SignalStrength"
        capability "Actuator"

        attribute "hubStatus", "ENUM", ["online","offline","unknown"]
	}
   
    section("Logging") 
    {
        input "enableLogging", "bool", title: "Enable debug logging for 30 minutes", multiple: false, defaultValue: true
    }
}

// parse events into attributes
def parse(String description)
{
    //TODO set hubStatus based in lwt
    def message = interfaces.mqtt.parseMessage(description)
    logTrace(message)

    if(message.topic.endsWith("/lwt"))
    {
        sendEvent(name: "hubStatus", value: message.payload)
        return
    }

    def slurper = new JsonSlurper()
    def attributes = slurper.parseText(message.payload)
    logTrace(attributes)

    if(attributes.containsKey("battery"))
    {
        sendEvent(name: "battery", value: attributes.battery)
    }

    if(attributes.containsKey("rssi"))
    {
        sendEvent(name: "rssi", value: attributes.rssi)
    }

    if(attributes.containsKey("position"))
    {
        sendEvent(name:"position", value: attributes.position)

        if(attributes.updateType == "updateRequested" || attributes.updateType == "moveComplete")
        {
            setShadeBasedOnPosition(attributes.position)
        }
        else if(attributes.updateType == "commandReceived")
        {
            sendEvent(name: "windowShade", value: state.lastMovementDirection);
        }
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
    state.lastMovementDirection = position > device.currentValue["position"] ? "opening" : "closing"
    publish("moveShade", true, [position:position])
}

def open()
{
    logTrace("Open")
    state.lastMovementDirection = "opening"
    publish("openShade", true)
}

def close()
{
    logTrace("Close")
    state.lastMovementDirection = "closing"
    publish("closeShade", true)
}

def startPositionChange(direction)
{
    "${direction}"()
}

def stopPositionChange()
{
    publish("stopShade", true)
}

def refresh()
{
    publish("updateDevice", false)
}

def publish(String command, Boolean includeKey)
{
    publish(command, includeKey, null)
}

/*
dooya_connector_hub/command                     {command:"updateDeviceList"}
                                                {command:"moveShade", mac:"<deviceMac>", key:"<hub-secret-key>", position:<newPosition>}
                                                {command:"openShade", mac:"<deviceMac>", key:"<hub-secret-key>"}
                                                {command:"closeShade", mac:"<deviceMac>", key:"<hub-secret-key>"}
                                                {command:"stopShade", mac:"<deviceMac>", key:"<hub-secret-key>"}
                                                {command:"updateDevce", mac:"<deviceMac>"}
                                                */

def publish(String command, Boolean includeKey, data)
{
    def details = parent.getConnectionDetails()
    def payload = [command:command, mac:details.mac]

    if(includeKey)
    {
        payload["key"] = details.hubKey
    }

    if(data)
    {
        payload << data
    }

    def topic = "${details.hubTopic}/command"
    logTrace("Publish: ${topic}, ${payload}")
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
}

def subscritionTopics()
{
    def details = parent.getConnectionDetails()
    def hubTopic = details.hubTopic
    def hubKey = details.key

    topics = []
    topics << "${details.hubTopic}/status/${details.mac}"
    topics << "${details.hubTopic}/lwt"
    return topics
}

public configure()
{
    logTrace("Broker details updated")
    if(interfaces.mqtt.isConnected())
    {
        subscritionTopics().each{ topic -> interfaces.mqtt.unsubscribe(topic) }
        interfaces.mqtt.disconnect()
    }
    reconectIfNessecary()
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
    def connectionDetails = parent.getConnectionDetails()
    if(!interfaces.mqtt.isConnected())
    {
        try
        {
            interfaces.mqtt.connect(connectionDetails.path, device.deviceNetworkId, connectionDetails.username, connectionDetails.password)
        }
        catch(exception)
        {
            logTrace("Connection error: " + exception)
            sendEvent(name: "hubStatus", value: "unknown")
        }
    }    
    
    subscritionTopics().each
    { 
        topic -> interfaces.mqtt.subscribe(topic)
        logTrace("Subscribed to: ${topic}")
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

