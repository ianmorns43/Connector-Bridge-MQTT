/**
 *  Dooya Window Shade (Hubitat)
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

definition(
    name: "Dooya Window Shade",
    namespace: "ianmorns_rfremote",
    parent: "ianmorns_rfremote:Connector Hub Control",
    author: "Ian Morns",
    description: "Connects to a device on a Dooya Connector Hub",
    category: "Convenience",
    iconUrl: "",
    iconX2Url: "",
    iconX3Url: "")


preferences
{  
    page(name: "mainPage")
}

def mainPage()
{    
    def page = dynamicPage(name: "mainPage",  title:"<b>Window Shade Setup</b>\n", install: true, uninstall: true)
    {
        section ()
        {
            input "blindName", "text", title: "Blind Name", multiple: false, required: true
            
            if(!getChildDevice())
            {
                //Once the devic is created you can't change its type without deleting it and creating a new one
                input "bidirectional", "bool", title: "Is blind bi-directional?", required: true
            }
            input "mac", "text", title: "The blind's MAC address on the connector hub", required: true;            
        }
               
        section()
        {
            input "enableLogging", "bool", title: "Enable debug logging for 30 minutes", multiple: false, defaultValue: false
        }
    }

    return page;
}

def installed() 
{
	log.debug "Installed with settings: ${settings}"

	initialize()
}

def updated() 
{
	log.debug "Updated with settings: ${settings}"

    unschedule();
	unsubscribe()
	initialize()
}

def uninstalled() 
{
    def allDevices = app.getChildDevices();
    allDevices.each{ device -> app.deleteChildDevice(device.getNetworkId);}
    log.debug "Uninstalled: ${app?.label}"
}

def initialize() 
{   
    logTrace("App ID = ${app.id}");
    logTrace("App Label ${app.label}");
    
    app.updateLabel(settings.blindName);

    def child = getChildDevice()
    if(!child)
    {
        if(settings.bidirectional)
        {
            child = app.addChildDevice("ianmorns_rfremote", "Dooya Bidirectional Blind", getChildDeviceId(), [label: settings.blindName, isComponent: true])
        }
        else
        {
            child = app.addChildDevice("ianmorns_rfremote", "Dooya Bidirectional Blind", getChildDeviceId(), [label: settings.blindName, isComponent: true])
        }
    }

    
    if(app.getChildDevices().any{ device -> device.data.remoteId != settings.remoteId})
    {
        updateConnections()        
    }
     
    if(!!enableLogging)   
    {
        runIn(1800, "disableLogging");
    }
}

private unassignedChannel(Number channel)
{
    return [channel: channel]
}

public updateConnections()
{
    logTrace("updateConnections: ")
    app.getChildDevices().each{ device -> device.configure() }
}

public brokerStatusChanged(evt)
{
    app.getChildDevices().each{ device -> device.brokerStatusChanged(evt) }
}

public getConnectionDetails()
{
    def details = parent.getHubDetails()
    details['mac'] = settings.mac
    return details
}

private getChildDeviceId()
{
    if(!state.controllerId)
    {
        state.controllerId = UUID.randomUUID().toString();
    }

    return state.controllerId;
}

private getChildDevice()
{
    return app.getChildDevice(getChildDeviceId());
}

private disableLogging()
{
    app?.updateSetting("enableLogging", false);
}

void logTrace(msg)
{
    if(settings.enableLogging)
    {
        log.trace msg;
    }
}