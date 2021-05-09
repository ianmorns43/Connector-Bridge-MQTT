 import groovy.json.JsonSlurper
 import groovy.json.JsonOutput

 /**
 *  Dooya Blind
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
	definition (name: "Dooya Blind", namespace: "ianmorns_rfremote", author: "Ian Morns") {
		capability "WindowShade"
        capability "Configuration"
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
    }

    if(message.topic.endsWith("/sent"))
    {
        sendEvent(name: "hubStatus", value: "online")
        def command = message.payload;

        /*if(command == "stop")
        {
            def current = device.currentValue("windowShade")
            if(current == "open" || current == "closed")
            {
                command = current
            }
            else
            {
                command = "partially open"
                unschedule("updateWindowShade")
                sendEvent(name: "windowShade", value: command)
            }
        }*/  

        
        if(command.endsWith("}"))
        {
            def parameters = new JsonSlurper().parseText(command)
            logTrace("Command ${command}")
            sendEvent(name: "windowShade", value: parameters.newWindowShade);
            sendEvent(name: "position", value: parameters.newPosition);

            runIn((Long) openCloseTime(parameters.newPosition), "updateWindowShade")
        }
    }
}

private openCloseTime(newPosition)
{
    def currentPosition = device.currentValue("position")
    logTrace("Diff (s) ${Math.abs(newPosition-currentPosition) / 100 * state.openCloseTime}")

    return (Long) Math.abs(newPosition-currentPosition) / 100 * state.openCloseTime
}

private updateWindowShade()
{
    def windowShade = "partially open"
    def currentPosition = device.currentValue("position")
    if(currentPosition == 100)
    {
        windowShade = "open"
    }
    else if (currentPosition == 0)
    {
        windowShade = "closed"
    }

    sendEvent(name: "windowShade", value: windowShade)
    logTrace("Update ${windowShade}")
}

def setPosition(position)
{
    logTrace("SetPosition: ${position}");
    
    if(position < 10)
    {
        sendMoveCommand(0, "closeSignal")
    }
    else if(position > 90)
    {
        sendMoveCommand(100, "openSignal")
    }
    else if(state.has3rdPosition)
    {   
        sendMoveCommand(50, "3rdPositionSignal")
    }
    else
    {
        //Try to move the blind by a timed "stop" message
        timedBlindMove(position)
    }
}

private timedBlindMove(position)
{
    def movementTime = openCloseTime(position)
    //Don't try and do really small moves
    if(movementTime < 2)
    {
        return
    }

    sendMoveCommand(position)

    runIn((Long) movementTime, "stopPositionChange")
}

private sendMoveCommand(position)
{
    def signal = device.currentValue("position") > position ? "closeSignal":"openSignal"
    sendMoveCommand(position, signal)
}

private sendMoveCommand(position, signal)
{
    def newWindowShade = device.currentValue("position") > position ? "closing":"opening"
    sendCommand(virtual(signal), JsonOutput.toJson([newPosition: position, newWindowShade: newWindowShade]))
}

def open()
{
    logTrace("Open");
    setPosition(100);
}

def close()
{
    logTrace("Close");
    setPosition(0);
}

def startPositionChange(direction)
{
    "${direction}"()
}

def stopPositionChange()
{
    sendCommand(virtual("stopSignal"), "stop");
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
    topics = []
    topics << "rf433/${remoteAndChannelID()}/#"
    if(getChannelNumber() != 0)
    {
        topics << "rf433/${remoteAndChannelID(0)}/#"
    }
    topics << "rf433/${device.data.hubMqttTopic}/lwt"
    return topics
}

def remoteAndChannelID()
{
    return remoteAndChannelID(getChannelNumber())
}

def remoteAndChannelID(channel)
{
    return "${device.data.remoteId.toLowerCase()}${channel}"
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
    device.updateDataValue("hubMqttTopic", connectionDetails.hubTopic)
    device.updateDataValue("remoteId", connectionDetails.remoteId)
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

private createPulse(Integer time, String level)
{
    return ["time_us": time, "level": level]
}

private createBit(Integer time_high, Integer time_low)
{
    def bit = [];
    if(time_high > 0)
    {
        bit << createPulse(time_high, "high")
    }
    bit << createPulse(time_low, "low")

    return bit
}

private command(ArrayList frames)
{
    return command(frames, state.broadcasts)
}

private command(ArrayList frames, Number broadcasts)
{
    return [broadcasts: broadcasts, frames: frames]
}

private hexStringToIntArray(String hexString)
{
    def data = []
    for(int i = 0; i < hexString.length(); i+=2)
    {
        data << Integer.parseInt(hexString.substring(i, i+2), 16)
    }

    return data
}

public setChannelInfo(channelInfo)
{
    state.channel = channelInfo.channel
    state.openCloseTime = channelInfo.openCloseTime
    state.has3rdPosition = channelInfo.has3rdPosition
    state.model = channelInfo.model
    state.broadcasts = channelInfo.broadcasts
    device.setLabel(channelInfo.name)

    logTrace("Channel set to ${state.channel}")
}

private Integer getChannelNumber()
{
    return state.channel
}

def virtual(method)
{
    return "${state.model}_$method"()
}

def dooya_openSignal()
{
    return dya_shortCommand("1E")
}

def dooya_closeSignal()
{
    return dya_shortCommand("3C")
}

def dooya_stopSignal()
{
    return dya_shortCommand("55")
}

def dooya_3rdPositionSignal()
{
    return dya_longCommand("55")
}

private dya_shortCommand(String commandCode)
{
    def repeats = getChannelNumber() == 0 ? 10 : 5
    return dya_command(commandCode, repeats, state.broadcasts)
}

private dya_longCommand(String commandCode)
{
	return dya_command(commandCode, 50, 1)
}

private dya_command(String commandCode, Number repeats, Number broadcasts)
{
	return command([dya_commandFrame(commandCode, repeats)], broadcasts)
}

private dya_commandFrame(String commandCode, Integer repeats)
{
    def dataString = (device.data.remoteId + getChannelNumber() + commandCode);
    logTrace("Data: ${dataString}")
    def data = hexStringToIntArray(dataString)

    def frame =
    [
        "repeats":repeats,
        "leadin":createBit(4800,1515),
        "leadout":createBit(0,9000),
        "one":createBit(723,370),
        "zero":createBit(370,723),
        "data":data
    ]

    logTrace("Frame: ${frame}")
    return frame;
}

//To fully open or close the blind if additional limits have been set, you have to double press the up or down button on the remote
def zemismart_openSignal()
{
    return zem_doublePress(0x0B)
}

def zemismart_closeSignal()
{
    return zem_doublePress(0x43)
}

//If additional limits have been set, a single press of the up or down button moves to the next limit position
def zem_upCommand()
{
    return zem_singlePress(0x0B)
}

def zem_downCommand()
{
    return zem_singlePress(0x43)
}

def zemismart_stopSignal()
{
    return command([zem_spacerFrame(), zem_commandFrame(0x23)])
}

def zemismart_3rdPositionSignal()
{
    def currentPosition = device.currentValue("position")
    if(currentPosition < 10)
    {
        return zem_upCommand()
    }
    if(currentPosition > 90)
    {
        return zem_downCommand()
    }

    return null
}

def zem_doublePress(Integer commandCode)
{
    return command([zem_spacerFrame(), zem_commandFrame(commandCode), zem_spacerFrame(), zem_interuptedCommandFrame(), zem_commandFrame(commandCode), zem_spacerFrame(), zem_commandFrame(0x24)])
}

def zem_singlePress(Integer commandCode)
{
    return command([zem_spacerFrame(), zem_commandFrame(commandCode), zem_spacerFrame(), zem_commandFrame(0x24)])
}

private zem_interuptedCommandFrame()
{
    return zem_commandFrame(0x24, true)
}

private zem_commandFrame(Integer commandCode)
{
    return zem_commandFrame(commandCode, false)
}

/*Parameters
* commandCode: the code for the button which was pressed, 0x0B = Up, 0x43 = down, 0x24 is the common code added after up and down
* truncate: if true will make a frame which looks like the last frame of an interupted signal. The frame is interupted suring the leadout.
            this mimickes what happpens when you double press a button on the remote.
*/
private zem_commandFrame(Integer commandCode, Boolean truncate)
{
    def data = hexStringToIntArray(device.data.remoteId)
    def c = zem_channelBytes()
    data << c[0]
    data << c[1]
    data << commandCode

    Integer parityByte = -163
    //See 'zemismart signal description.md' for more info on this calculation
    data.each{ b -> parityByte += (Byte) b}
    
    if(parityByte<0)
    {
        parityByte += 256
    }

    data << parityByte
    logTrace(data)

    return [
        "repeats": truncate ? 1 : 6,
        "leadin":createBit(5000,614),
        "leadout":createBit(600, truncate ? 2000 : 5000),
        "one":createBit(600,276),
        "zero":createBit(276,600),
        "data":data
    ] 
}

private zem_spacerFrame()
{
    return [
        "repeats":1,
        "leadin":[],
        "leadout":[],
        "one":[],
        "zero":createBit(276,600),
        "data":[0]
    ]
}

private zem_channelBytes()
{
    if(getChannelNumber() == 0)
    {
        return [255,255]
    }
    if(state.channel < 9)
    {
        return [(Integer) Math.pow(2,getChannelNumber()-1), 0]
    }

    return [0, (Integer) Math.pow(2,getChannelNumber()-9)]
}

def zemismart_tuya_openSignal()
{
    return ztCommand(["26EB5C6227B15C4A5D", "37FA4D7336A04D5B4D", "04C97E4005937E6879", "15D86F5114826F7969", "62AF182663F5180E1D"])
}

def zemismart_tuya_closeSignal()
{
    return ztCommand(["458D3A0441D73A2C31", "549C2B1550C62B3D24", "BA72C4FBBE28C4D3CB", "AB63D5EAAF39D5C2D9", "9850E6D99C0AE6F1EB"])
}

def zemismart_tuya_stopSignal()
{
    return ztCommand(["9E50EAD99C0AEAF13C", "8F41FBC88D1BFBE02F", "438D340441D7342CE2", "529C251550C6253DF4", "61AF162663F5160EC6"])
}

def zemismart_tuya_3rdPositionSignal()
{
    //Not implemented
}

private ztCommand(ArrayList codes)
{
    def frames = []
    codes.each{ code -> frames << ztFrame(code)}

    return command(frames, 1)
}

private ztFrame(String code)
{
    def data = hexStringToIntArray(code)
    logTrace("Data: ${code} -> ${data}")


    return [
        "repeats":1,
        "leadin":createBit(2048, 4198),
        "leadout":createBit(492,6390),
        "one":createBit(592,1073),
        "zero":createBit(382,451),
        "data":data
    ] 
}

private sendCommand(Map signalData, String commandLabel)
{
    if(!signalData?.frames)
    {
        logTrace("There's nothing to send")
        //updateWindowShade(["windowShade": commandLabel])
        return
    }

    def signal =
    [
        "remoteid":remoteAndChannelID(),
        "signalid":commandLabel,
        "frames":  signalData.frames
    ]

	//logTrace(signal)
	broadcast(signal, signalData.broadcasts)
}

def rebroadcast(data)
{
    data.signal["signalid"] = "rebroadcast"
    broadcast(data.signal, data.broadcasts)
}

def broadcast(Map signal, Number broadcasts)
{
    logTrace("Broadcasts to do: ${broadcasts}")
    reconectIfNessecary()
    def topic = "rf433/${device.data.hubMqttTopic}/send"

    def payload = JsonOutput.toJson(signal)
    logTrace("${topic} - ${payload}")
    interfaces.mqtt.publish(topic, payload)

    if(--broadcasts > 0)
    {
        runIn(5, rebroadcast, [data: [signal: signal, broadcasts: broadcasts]])
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

