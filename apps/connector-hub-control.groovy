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
    page(name: "confirmBlindAddedPage")
    page(name: "removeBlindsPage")
    page(name: "removeBlindsAreYouSurePage")
    page(name: "confirmBlindsRemoved")
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
            section("<b>Remove blinds</b>")
            {
                href "removeBlindsPage", title: "<b>Remove Blinds</b>", description: "Select blinds to unmount from Hubitat."
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
    def unasignedDevices = getDevicesAvailableToAdd()

    def nextOrDone = settings.blindToAdd && settings.blindName
    def paramValue = nextOrDone ? "confirmBlindAddedPage" : "mainPage"

    state.blindAdded = false    
	def page = dynamicPage(name: "addBlindsPage",  title:"<b>Select a blind to add</b>\n", nextPage: paramValue)
    {


        def deviceCount = unasignedDevices.size()

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
            }

            if(settings.blindToAdd && settings.blindName)
            {
                paragraph "Read to add '${settings.blindName}'. Click 'Next' to add the blind."
            }
            else
            {
                paragraph "Complete the required fields or click 'Next' to return to the main page."
            }
        }

        logTrace("Blinds to add: ${blindToAdd}")
    }

    return page
}

def confirmBlindAddedPage()
{   
	def page = dynamicPage(name: "confirmBlindAddedPage",  title:"<b>Blind added</b>\n", nextPage: "mainPage")
    {       
        section()
        {
            logTrace ("Building")
            def newBlindName = settings.blindName
            if(!state.blindAdded && settings.blindToAdd && settings.blindName)
            {
                logTrace ("Adding ${settings.blindName}")
                def child = addChildDevice("ianmorns_rfremote", "Dooya Bidirectional Blind", UUID.randomUUID().toString(), [label: settings.blindName, isComponent: true])
                child.setMac(settings.blindToAdd)

                app.updateSetting("addTheBlind", false)
                app.removeSetting("blindName")
                app.removeSetting("blindToAdd")
                state.blindAdded = true
            }
            
            if(state.blindAdded)
            {
                paragraph "Blind ${newBlindName} added successfully. Click 'Next' to return to previous page."
            }
            else
            {
                paragraph "Something went wrong! Click 'Next' to return to previous page."
            }
        }

        logTrace("Blinds to add: ${blindToAdd}")
    }

    return page
    
}

def removeBlindsPage()
{
    def activeBlinds = blindsWhichCanBeDeleted(true)
    def inactiveBlinds = blindsWhichCanBeDeleted(false)

    def anyBlindSelected = !!settings.activeBlindsToDelete || !!settings.inactiveBlindsToDelete
    def paramValue = anyBlindSelected ? "removeBlindsAreYouSurePage" : "mainPage"

    
	def page = dynamicPage(name: "removeBlindsPage",  title:"<b>Select blinds to delete</b>\n", nextPage: paramValue)
    {
        section("<b>Active Blinds</b>")
        {
            def count = activeBlinds.size()
            input ("activeBlindsToDelete", "enum",
                        required: false,
                        multiple: true,
                        title: "There ${count == 1? "is":"are"} ${count} active ${count == 1? 'blind':'blinds'} currently installed on Hubitat",
                        description: "Use the dropdown to select blinds to delete.",
                        submitOnChange: true,
                        options: activeBlinds)
        }

        count = inactiveBlinds.size()
        if(count > 0)
        {
            section("<b>Inactive Blinds</b>")
            {
                paragraph "You have inactive blinds installed in Hubitat. These are blinds with MACs which do not exist on the Connector Hub. This could be because the blind was" +
                            " deleted from the Connector Hub or because its MAC has been edited. Commands sent from these blinds will be rejected by the Hub. You can either go to the" +
                            " blind device and fix the MAC or delete the blind here."
                input ("inactiveBlindsToDelete", "enum",
                            required: false,
                            multiple: true,
                            title: "There ${count == 1? "is":"are"} ${count} inactive ${count == 1? 'blind':'blinds'} currently installed on Hubitat",
                            description: "Use the dropdown to select blinds to delete.",
                            submitOnChange: true,
                            options: inactiveBlinds)
            }
        }

        section()
        {
            if(anyBlindSelected)
            {
                paragraph "Click 'Next' to confirm and delete blinds."
            }
            else
            {
                paragraph "Click 'Next' to return to main page."
            }
        }
    }

    return page
    
}

def removeBlindsAreYouSurePage()
{
    state.blindsRemoved = false

    def page = dynamicPage(name: "removeBlindsAreYouSurePage",  title:"<b>Confirm Delete</b>\n", nextPage: "confirmBlindsRemoved")
    {
        section()
        {
            def blindsToDeleteCount = (settings.activeBlindsToDelete?.size() ?: 0) + (settings.inactiveBlindsToDelete?.size() ?: 0)
            paragraph "Ready to remove ${blindsToDeleteCount} blind${blindsToDeleteCount > 1 ? 's':''}. Warning, if you continue, any automations using these blinds may no longer function correctly."

            paragraph "Click 'Next' to delete blinds."
            paragraph "Click Back to return to previous page."
        }
    }

    return page
    
}

def confirmBlindsRemoved()
{
	def page = dynamicPage(name: "confirmBlindsRemoved",  title:"<b>Delete Complete</b>\n", nextPage: "mainPage")
    {
        section()
        {
            def removed = 0
            if(!state.blindsRemoved)
            {
                settings.activeBlindsToDelete?.each{app.deleteChildDevice(it)}
                settings.inactiveBlindsToDelete?.each{app.deleteChildDevice(it)}
                removed = (settings.activeBlindsToDelete?.size() ?: 0) + (settings.inactiveBlindsToDelete?.size() ?: 0)
                state.blindsRemoved = true

                app.removeSetting("activeBlindsToDelete")
                app.removeSetting("inactiveBlindsToDelete")
            }

            paragraph "${removed} blind${removed > 1 ? 's':''} deleted."
            paragraph "Click 'Next' to return to main page."
        }
    }

    return page
    
}

private blindsWhichCanBeDeleted(active)
{
    def activeMacs = getHubDevice().getDeviceList().collect{ it.mac }
    logTrace("Active Macs ${activeMacs}")

    def allBlinds = app.getChildDevices().findAll{it.deviceNetworkId != getHubDeviceId() }

    logTrace("All blinds ${allBlinds}")
    def blinds = [:]

    allBlinds.findAll{blind -> (!active) ^ activeMacs.any{mac -> mac == blind.getMac()}}.each
    { blind ->
        blinds[blind.deviceNetworkId] = blind.label
    }

    return blinds
}

def getDevicesAvailableToAdd()
{
    def deviceList = getHubDevice().getDeviceList()
    def unasignedDevices = [:]
    deviceList.findAll{it.isBidirectional}.findAll{ !app.getChildDevices().any{child -> child.getMac() == it.mac}}.each{ unasigned ->
        unasignedDevices[unasigned.mac] = "Mac: ${unasigned.mac}, Type: Bi-directional ${unasigned.shadeType}"}

    return unasignedDevices
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