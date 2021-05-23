/**
 *  <Shade Group Controller> (Hubitat)
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
    name: "Shade Group Controller",
    namespace: "ianmorns_connector",    
    parent: "ianmorns_connector:Shade Group Manager",
    author: "Ian Morns",
    description: "Group a set of shades which will all receive the command sent to a parent shade",
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
	def page = dynamicPage(name: "mainPage",  title:"<b><Settings></b>\n", uninstall: true, install: true)
    {       
        section("<b>Name<b>") 
        {
            input "appName", "text", title: "Set name for this Shade Group", multiple: false, required: true
        }
        section("<b>Name<b>") 
        {
            input "shades", "capability.windowShade", title: "Select shades which will act as a group", multiple: true, required: true
        }
        section("Logging") 
        {
            input "enableLogging", "bool", title: "Enable trace logging for 30 minutes", multiple: false, defaultValue: false
        }
    }
    
    return page;
}

def installed() 
{
	logTrace("Installed with settings: ${settings}");

	initialize()
}

def updated() 
{
	logTrace("Updated with settings: ${settings}");
    
    unschedule();
	unsubscribe()
	initialize()
}

def uninstalled() 
{
    if(getChildDevice())
    {
        app.deleteChildDevice(getChildDeviceId());
    }
    
    logTrace("Uninstalled: ${app?.label}")
}

def initialize() 
{
    if(!!enableLogging)
    {
        runIn(1800, "disableLogging");
    }
    
    def child = getChildDevice();
    if(!child)
    {
        child = app.addChildDevice("ianmorns_connector", "Shade Group", getChildDeviceId(), [label: appName, isComponent: true])
    }
    
    child.setLabel(settings.appName);
    app.updateLabel(settings.appName);

    subscribe(settings.shades, "position", positionChanged)
    subscribe(settings.shades, "windowShade", shadeChanged)     
}

def positionChanged(evt)
{
    def total = 0;
    settings?.shades.each{total += it.currentValue("position")}

    getChildDevice()?.sendEvent(name: "position", value: (Integer) (total / settings?.shades.size()))
}

def shadeChanged(evt)
{
    def variants = settings.shades?.unique{it.currentValue("windowShade")}
    def windowShade = variants.size() == 1 ? variants.first().currentValue("windowShade") : "partially open"
    getChildDevice()?.sendEvent(name: "windowShade", value: windowShade)
}

public open()
{
    settings.shades?.each{it.open()}
}

public close()
{
    settings.shades?.each{it.close()}
}

public stopPositionChange()
{
    settings.shades?.each{it.stopPositionChange()}
}

public setPosition(position)
{
    settings.shades?.each{it.setPosition(position)}
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