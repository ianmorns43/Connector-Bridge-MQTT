/**
 *  Shade Group Manager (Hubitat)
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
    name: "Shade Group Manager",
    namespace: "ianmorns_connector",    
    singleInstance: true,
    author: "Ian Morns",
    description: "<DESCRIPTION>",
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
    def page = dynamicPage(name: "mainPage",  title:"<b>Setup</b>\n", uninstall: true, install: true)
    {
        section()
        {
            app(name: "childApps", appName: "Shade Group Controller", namespace: "ianmorns_connector", title: "Add a new shade group", multiple: true)
        }
        section()
        {
            input "enableLogging", "bool", title: "Enable trace logging for 30 minutes", multiple: false, defaultValue: false
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
    log.debug "Uninstalled: ${app?.label}"
}

def initialize() 
{       
    if(!!enableLogging)
    {
        runIn(1800, "disableLogging");
    }
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