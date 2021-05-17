/**
 *  Connector Hub Control (Hubitat)
 *
 *  Copyright 2021 Ian Morns
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
    name: "Connector Hub Control",
    namespace: "ianmorns_rfremote",    
    singleInstance: true,
    author: "Ian Morns",
    description: "Proxy to Dooya Connector Hub",
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
    def validationResult = validateBroker()
    updateHubDevice()
    
	def page = dynamicPage(name: "mainPage",  title:"<b>Connector Hub Setup</b>\n", uninstall: true, install: true)
    {
        section("Connector Hub Setup")
        {
            input "hubName", "text", title: "Hub Name", multiple: false, submitOnChange: true, required: true
            input "hubTopic", "text", title: "Hub MQTT Topic", multiple: false, submitOnChange: true, required: true, defaultValue: "DD7002B"
            input "hubKey", "password", title: "Hub Key", multiple: false, required: true, description: "to get your key, open 'Connector' app on your mobile device. Go to 'About' and tap the connector icon 5 times."
        }
        section("Mqtt Broker Setup")
        {
            input "mqttBroker", "capability.healthCheck", title: "MQTT Broker Device", multiple: false, required: true, submitOnChange: true
            if(!settings.brokerValidationResult)
            {
                paragraph "To be valid, the selected Broker must be a device of type 'MQTT Broker'."
            }
            input "brokerValidationResult", "text", title: "", multiple: false, required: true, submitOnChange: true, description: validationResult
        }

        section("<b>Window Shade Remotes</b>")
        {
            app(name: "windowShadeRemote", appName: "Dooya Window Shade", namespace: "ianmorns_rfremote", title: "Add a new window shade remote", multiple: true)
        }

        section()
        {
            input "enableLogging", "bool", title: "Enable debug logging for 30 minutes", multiple: false, defaultValue: false
        }
    }
    
    return page;
}

def updateHubDevice()
{
    def hubDevice = getHubDevice()
    if(!hubDevice && settings.brokerValidationResult && settings.hubTopic && settings.hubName)
    {
        hubDevice = app.addChildDevice("ianmorns_rfremote", "Connector Hub", getHubDeviceId(), [label: settings.hubName, isComponent: true])
        hubDevice.configure()
    }
}

def validateBroker()
{
    def attributes = settings.mqttBroker?.getSupportedAttributes()
    def broker = settings.mqttBroker;
    def valid = broker?.hasAttribute('brokerDetails') && broker?.hasAttribute('brokerStatus') && broker?.hasCommand("getBrokerDetails")
    logTrace("Is a valid Broker device? " + valid)

    if(valid)
    {
        app?.updateSetting("brokerValidationResult", "The selected type is a valid Broker.")
        return "The selected type is a valid Broker."
    }
    else
    {
        app?.removeSetting("brokerValidationResult")
        return !settings.mqttBroker ? "No Broker device is selected." : "The selected device is of the wrong type."
    }
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

    app.updateLabel(settings.hubName);

    subscribe(settings.mqttBroker, "brokerDetails", updateConnections)
    subscribe(settings.mqttBroker, "brokerStatus", brokerStatusChanged)

    if(settings.hubTopic != app.state.previoushubTopic)
    {
        app.state.previoushubTopic = settings.hubTopic;
        updateConnections(null)
    }
}

public updateConnections(evt)
{
    logTrace("updateConnections: " + evt)
    app?.getChildApps().each{ it.updateConnections() }
}

public brokerStatusChanged(evt)
{
    app.getChildApps().each{ it.brokerStatusChanged(evt) }
    app.getChildDevices().each{ it.brokerStatusChanged(evt) }
}

public getHubDetails()
{
    def details = [:]
    settings.mqttBroker.getBrokerDetails(details)
    details['hubTopic'] = settings.hubTopic
    details['hubKey'] = settings.hubKey
    return details
}

private getHubDeviceId()
{
    if(!state.hubDeviceId)
    {
        state.hubDeviceId = UUID.randomUUID().toString();
    }

    return state.hubDeviceId;
}

private getHubDevice()
{
    return app.getChildDevice(getHubDeviceId());
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