 /**
 *  Shade Group (Hubitat)
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
	definition (name: "Shade Group", namespace: "ianmorns_connector", author: "Ian Morns") {
		capability "WindowShade"
        capability "Switch"
        //https://docs.hubitat.com/index.php?title=Driver_Capability_List
	}
    
    section("Logging") 
    {
        input "enableLogging", "bool", title: "Enable trace logging for 30 minutes", multiple: false, defaultValue: false
    }
}

// parse events into attributes
def parse(String description) 
{   
}

//Part of capability.switch
def setPosition(position)
{
    logTrace("SetPosition: ${position}");
    parent.setPosition(position)
}

def open()
{
    logTrace("Open")
    parent.open()
}

def close()
{
    logTrace("Close")
    parent.close()
}

def on()
{
    logTrace("On")
    parent.on()
}

def off()
{
    logTrace("Off")
    parent.off()
}

def startPositionChange(direction)
{
    "${direction}"()
}

def stopPositionChange()
{
    parent.stopPositionChange()
}

def installed()
{
    initialize();
}

def updated()
{
    unschedule();
    initialize();
}

def initialize()
{
    if(settings.enableLogging)
    {
        runIn(1800, "disableLogging");
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

