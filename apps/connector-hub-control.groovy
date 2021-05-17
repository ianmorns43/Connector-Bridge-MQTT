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
    page(name: "addBlindsPage")
}

def mainPage()
{
    def validationResult = validateBroker()
    def hubDevice = updateHubDevice()
    
	def page = dynamicPage(name: "mainPage",  title:"<b>Setup</b>\n", uninstall: true, install: true)
    {
        def hubIp = location?.hubs[0]?.localIP
        def title = (hubIp && hubDevice) ? "<b><a href='http://${hubIp}/device/edit/${hubDevice.id}' target='_blank'>Connector Hub</a><b>" :
                    "<b>Connector Hub</b>"

        section(title)
        {
            input "hubName", "text", title: "Hub Name", multiple: false, submitOnChange: true, required: true
            input "hubTopic", "text", title: "Hub MQTT Topic", multiple: false, submitOnChange: true, required: true, defaultValue: "DD7002B"
            input "hubKey", "password", title: "Hub Key", multiple: false, required: true, description: "to get your key, open 'Connector' app on your mobile device. Go to 'About' and tap the connector icon 5 times."
        }
        section("<b>Mqtt Broker</b>")
        {
            input "mqttBroker", "capability.healthCheck", title: "MQTT Broker Device", multiple: false, required: true, submitOnChange: true
            if(!settings.brokerValidationResult)
            {
                paragraph "To be valid, the selected Broker must be a device of type 'MQTT Broker'."
            }
            input "brokerValidationResult", "text", title: "", multiple: false, required: true, submitOnChange: true, description: validationResult
        }

        //TODO Some info about whether the hub is up to date or not

        if(hubDevice)
        {
            section("<b>Add blinds</b>")
            {
                href "addBlindsPage", title: "<b>Add Blinds</b>", description: "Select blinds to mount on Hubitat."
            }
        }

        section("Logging")
        {
            input "enableLogging", "bool", title: "Enable debug logging for 30 minutes", multiple: false, defaultValue: false
        }
    }
    
    return page;
}


def addBlindsPage()
{
    def deviceList = getHubDevice().getDeviceList()
    //Only doing biDirecitonal motors for now. Unidirectional wont have shadeType
    def unasignedDevices = [:]
    
    deviceList.findAll{it.isBidirectional}.findAll{ !app.getChildDevices().any{child -> child.getMac() == it.mac}}.each{ unasigned ->
        unasignedDevices[unasigned.mac] = "Mac: ${unasigned.mac}, Type: Bi-directional ${unasigned.shadeType}"

    }

    def deviceCount = unasignedDevices.size()
    
	def page = dynamicPage(name: "addBlindsPage",  title:"<b>Select a blind to add</b>\n", uninstall: false, install: false)
    {
        section()
        {
            input ("blindToAdd", "enum",
                        required: false,
                        multiple: false,
                        title: "There ${deviceCount == 1? "is":"are"} ${deviceCount} ${deviceCount == 1? 'blind':'blinds'} which may be installed on Hubitat",
                        description: "Use the dropdown to select blind to add Hubitat.",
                        submitOnChange: true,
                        options: unasignedDevices)

            if(settings.blindToAdd)
            {
                input "blindName", "text", title: "Blind name", required: true, submitOnChange: true
            

                if(settings.blindName)
                {
                    //TODO check blind type, currently there is only bidirectional
                    input "addTheBlind", "bool", title:"Click to add this blind", submitOnChange: true, defaultValue: false

                    if(settings.addTheBlind)
                    {
                        def child = addChildDevice("ianmorns_rfremote", "Dooya Bidirectional Blind", UUID.randomUUID().toString(), [label: settings.blindName, isComponent: true])
                        child.setMac(settings.blindToAdd)
                        app.updateSetting("addTheBlind", false)
                        app.removeSetting("blindName")
                        app.removeSetting("blindToAdd")
                    }
                }
            }
        }

        logTrace("Blinds to add: ${blindToAdd}")
    }

    return page
    
}

def updateHubDevice()
{
    def hubDevice = getHubDevice()
    if(!hubDevice && settings.brokerValidationResult && settings.hubTopic && settings.hubName)
    {
        hubDevice = app.addChildDevice("ianmorns_rfremote", "Connector Hub", getHubDeviceId(), [label: settings.hubName, isComponent: true])
        hubDevice.configure()
    }

    return hubDevice
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