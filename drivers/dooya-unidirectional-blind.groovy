 /**
 *  Dooya Unidirectional Blind
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
	definition (name: "Dooya Unidirectional Blind", namespace: "ianmorns_connector", author: "Ian Morns") {
		capability "WindowShade"
        capability "Switch"
        capability "Actuator"

        attribute "hubStatus", "ENUM", ["online","offline","unknown"]
	}
   
    section() 
    {
        input "openCloseTime", "number", title: "Time blind takes to fully open or close (s)", required: true
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
    logTrace("At device: ${message}")
    if(message.messageType == "lwt")
    {
        sendEvent(name: "hubStatus", value: message.payload)
        return
    }

    if(message.payload.containsKey("shadeType"))
    {
        device.setName(message.payload.shadeType)
    }

    sendEvent(name: "hubStatus", value: "online")

    if(message.messageType == "moved")
    {
        handleBlindMoved(message.payload)
    }
}

def handleBlindMoved(payload)
{
    logTrace("moved received: ${payload}")
    def operation = payload?.operation
    if(!state.movement)
    {
        logTrace("No movement recorded")
        if(operation == "open" || operation == "close")
        {
            //If not movement is recorded, movement was probably intiated from connector app
            state.movement = createMovementRecord(operation == "open" ? 100:0)
            logTrace("Movement created: ${state.movement}")
        }
    }

    if(!!state.movement)
    {
        logTrace("Movement: ${state.movement}")
        def movementTime
        if(operation == "stop")
        {
            state.movement.targetPosition = estimateCurrentPosition(state.movement)
            movementTime = 0
        }
        else
        {
            state.movement["started"] = new Date().time
            movementTime = estimateMovementTime(state.movement.targetPosition)
        }

        logTrace("Movement time: ${movementTime}")
        if(movementTime > 0)
        {
            sendEvent(name: "windowShade", value: state.movement.direction > 0 ? "opening" : "closing")
            runIn(movementTime, finalShadeAnPositionUpdate)
            
            if(state.movement.targetPosition != 100 && state.movement.targetPosition != 0)
            {
                runIn(movementTime, sendStopCommand)
            }
        }
        else
        {
            finalShadeAnPositionUpdate()
        }
    }
}

def finalShadeAnPositionUpdate()
{
    unschedule(sendStopCommand)
    unschedule(finalShadeAnPositionUpdate)
    sendEvent(name: "windowShade", value: state.movement.targetPosition == 100 ? "open" : (state.movement.targetPosition == 0 ? "closed":"partially open"))
    sendEvent(name: "position", value: state.movement.targetPosition)
    state.remove("movement")
}

def estimateMovementTime(position)
{
    def positionChange = position - (device.currentValue("position") ?:0)
    return (Integer) (Math.abs(positionChange) / 100 * settings.openCloseTime)
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
    def movementTime = estimateMovementTime(position)

    if(movementTime < 1 && position != 0 && position != 100)
    {
        return
    }

    state.movement = createMovementRecord(position)

    def positionChange = position - (device.currentValue("position") ?:0)

    if(position == 100 || positionChange > 0)
    {
        publishCommand([command:"openShade", includeKey:true])
    }
    else if(position == 0 || positionChange < 0)
    {
        publishCommand([command:"closeShade", includeKey:true])
    }
}

def createMovementRecord(position)
{
    def currentPosition = device.currentValue("position")
    return [startPosition: currentPosition, targetPosition: position, direction: currentPosition < position ? 1:-1]
}

def open()
{
    logTrace("Open")
    setPosition(100)
}

def close()
{
    logTrace("Close")
    setPosition(0)
}

def startPositionChange(direction)
{
    "${direction}"()
}

def stopPositionChange()
{
    sendStopCommand()
}

def estimateCurrentPosition(Map movement)
{   
    def elapsed = new Date().time - movement.started
    def positionChange = elapsed/(settings.openCloseTime * 1000.0) * 100
    def position = (Integer) Math.round(movement.startPosition + movement.direction * positionChange)

    position = Math.max(0,Math.min(100, position))

    return position
}

def sendStopCommand()
{
    publishCommand([command:"stopShade", includeKey:true])
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
    sendEvent(name: "position", value: 100)
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

