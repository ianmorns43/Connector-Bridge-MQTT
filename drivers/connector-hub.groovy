 import groovy.json.JsonSlurper
 import groovy.json.JsonOutput
 import java.util.regex.Pattern

 /**
 *  Connector Hub
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
	definition (name: "Connector Hub", namespace: "ianmorns_connector", author: "Ian Morns") {
        capability "Configuration"
        capability "Refresh"
        capability "Actuator"
        capability "SignalStrength"
        
        command "Reset"

        attribute "lastRefreshed", "text"
        attribute "deviceCount", "NUMBER"
        attribute "uptoDateDevices", "NUMBER"
        attribute "hubStatus", "ENUM", ["online","offline","unknown"]
	}
   
    section() 
    {
        input "enableLogging", "bool", title: "Enable debug logging for 30 minutes", multiple: false, defaultValue: true
    }
}

def parse(String description)
{
    def message = interfaces.mqtt.parseMessage(description)
    logTrace(message)

    def parts = message.topic.split('/')
    logTrace("${parts?.size()} parts: ${parts}")
    if(parts?.size() != 3)
    {
        return
    }

    //def root = parts[0]
    //def deviceType = parts[1] <hub|blind>
    def messageType = parts[2]

    if(messageType == "lwt")
    {
        sendEvent(name: "hubStatus", value: message.payload)
        return
    }

    def slurper = new JsonSlurper()
    def payload = slurper.parseText(message.payload)
    logTrace(payload)

    if(messageType == "heartbeat")
    {
        sendEvent(name:"rssi", value:payload.rssi)
        if(payload.deviceCount != device.currentValue("deviceCount"))
        {
            refresh()
        }
        return
    }

    if(messageType == "deviceList")
    {
        unschedule(refreshWithRetry)
        device.updateDataValue("mac", payload.mac)

        //Add any new devices to the stored list
        payload.deviceMacs.findAll{deviceMac -> !state.deviceList.any{it.mac==deviceMac} }?.each{ state.deviceList << [mac:it]}

        //If the 2 lists are different sizes, that means 1 or more devices have been removed. Remove them from our list
        if(state.deviceList.size() > payload.deviceMacs.size())
        {
            state.deviceList = state.deviceList.findAll{ stored -> payload.deviceMacs.any{macOnHub -> stored.mac == macOnHub}}
        }
       
        refreshDevicesWithRetry()
        return
    }

    if(messageType == "status")
    {
        try
        {            
            def deviceMac = payload.mac
            logTrace("deviceMac ${deviceMac}")

            state.deviceList.findAll{it.mac == deviceMac}.each
            {
                it.isBidirectional = payload.bidirectional

                if(payload.shadeType)
                {
                    it.shadeType = payload.shadeType
                }
            }
        }
        catch(ex)
        {
            logTrace("Error: ${ex}, stopping retry")
            unschedule(refreshDevicesWithRetry)
        }

        updateDeviceCounts()
    }

    parent.forwardMessage([messageType:messageType, payload:payload])
}

public getMac()
{
    return device.data.mac
}

public getDeviceList()
{
    return state.deviceList
}

def updateDeviceCounts()
{
    def deviceCount = state.deviceList.size()
    def upToDateCount = state.deviceList.count{it.isBidirectional != null}
    sendEvent(name: "deviceCount", value: deviceCount)
    sendEvent(name: "uptoDateDevices", value: upToDateCount)

    return [upToDateCount:upToDateCount, deviceCount:deviceCount]
}

def refreshDevicesWithRetry()
{
    def counts = updateDeviceCounts()
    if(counts.upToDateCount >= counts.deviceCount)
    {
        //All devices are uptodate, we're done
        sendEvent(name:"lastRefreshed", value: new Date(now()))
        return
    }

    state.deviceList.findAll{it.isBidirectional == null}?.each{publish([command:"getStatus", mac:it.mac])}  

    def retryDelay = (Integer)(20 + (1 + counts.deviceCount - counts.upToDateCount)/2)
    runIn(retryDelay, refreshDevicesWithRetry)
}

def Reset()
{
    state.deviceList = []
    updateDeviceCounts()
    refresh()

    //state.deviceList << [shadeType:"Roller Blinds", isBidirectional:true, mac:"f008d1edd4ec0001"]
    //updateDeviceCounts()
}

def refresh()
{
    //state.deviceList.each{it.isBidirectional = null}
    refreshWithRetry([retryInterval:10])
}

def refreshWithRetry(data)
{
    logTrace("RefreshRetry ${data}")
    logTrace("RefreshRetry ${data.retryInterval}")

    def maximumRetryInterval = 600
    def retryInterval = data.retryInterval
    data.retryInterval = Math.min((Integer) (retryInterval * 1.5), maximumRetryInterval)

    
    publish([command:"updateDeviceList"])
    runIn(retryInterval, refreshWithRetry, [data: data])
}

public publish(Map payload)
{
    reconectIfNessecary()

    def details = parent.getHubDetails()    
    def topic = "${details.hubTopic}/command"
    def payloadJson = JsonOutput.toJson(payload)    
    logTrace("Publish: ${topic}, ${payloadJson}")
    interfaces.mqtt.publish(topic, payloadJson)    
}

def installed()
{
    state.deviceList = []
    initialize();
}

def uninstalled()
{
    logTrace("Uninstalling")
    unsubscribeAndDisconnect()
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

    schedule("${second} ${minute}/${refreshInterval} * ? * * *", "reconectIfNessecary");
}

def subscritionTopics()
{
    def details = parent.getHubDetails()
    def hubTopic = details.hubTopic

    topics = []
    topics << "${details.hubTopic}/hub/+"
    topics << "${details.hubTopic}/blind/+"
    return topics
}

public configure()
{
    logTrace("Broker details updated")
    unsubscribeAndDisconnect()
    refresh()
}

private unsubscribeAndDisconnect()
{
    if(interfaces.mqtt.isConnected())
    {
        subscritionTopics().each{ topic -> interfaces.mqtt.unsubscribe(topic) }
        interfaces.mqtt.disconnect()
    }
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

