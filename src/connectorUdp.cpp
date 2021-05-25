#include "connectorUdp.h"
#include "deviceType.h"

typedef StaticJsonDocument<2048> JsonDocumentRoot;

ConnectorUdp::ConnectorUdp(udpMessageQueue& udpMessages)
:udpMessages(udpMessages)
{
    start();
}

void ConnectorUdp::start()
{  
    deviceListReceived = false;
    lastTimeDeviceListRequested = millis();
}

mqttMessage ConnectorUdp::loop()
{
    auto packet = udpMessages.readNextIncomingPacket();
    
    if (!packet.empty())
    {       
        JsonDocumentRoot doc;
        auto error = deserializeJson(doc, packet);
        if(error)
        {
            Serial.printf("Error: %s\r\n", error.c_str());
            return mqttMessage::emptyMessage();
        }

        auto messageType = std::string((const char *)doc["msgType"]);

        if(messageType == "Heartbeat")
        {
            auto hubMac = parseHubDetailsAndFindMac(doc);

            auto data = doc["data"];
            auto rssi = (int) data["RSSI"];
            auto deviceCount = (int )data["numberOfDevices"];

            return mqttMessage::createHeartbeatMessage(deviceCount, hubMac.c_str(), rssi);
        }
        else if(messageType == "GetDeviceListAck")
        {
            deviceListReceived = true;

            auto hubMac = parseHubDetailsAndFindMac(doc);
            auto data = doc["data"];
 
            mqttMessage::DeviceList deviceList;
            deviceList.reserve(data.size());

            for(size_t i = 0; i < data.size(); i++)
            {
                auto device = data[i];
                auto deviceType = (const char*)device["deviceType"];

                if(DeviceType::RFMotor == deviceType)
                {
                    auto deviceMac = (const char*) device["mac"];
                    deviceList.push_back(deviceMac);
                }
            }

            return mqttMessage::createDeviceListMessage(hubMac.c_str(), deviceList);

        }
        else if(messageType == "WriteDeviceAck")
        {
            return createDeviceMessage("moving", doc);            
        }
        else if(messageType == "Report") //When motor stops
        {
            return createDeviceMessage("moved", doc);
        }
        else if(messageType == "ReadDeviceAck") //When asked for
        {           
            return createDeviceMessage("status", doc);
        }

        Serial.printf("Unrecognised udp: %s\r\n", packet.c_str());
    }
    else if(!deviceListReceived && millis() > (lastTimeDeviceListRequested + 5000u))
    {
        lastTimeDeviceListRequested = millis();
        deviceListReceived = false;
        Serial.println("Asking hub to identify itself...");
        udpMessages.queueMulticastDeviceListRequest();
    }

    return mqttMessage::emptyMessage();
}

mqttMessage ConnectorUdp::createDeviceMessage(const char* updateType, JsonDocument& doc)
{
    auto data = doc["data"];
    if(data == nullptr)
    {
        //If you attempt to interact with a motor which does not exist you get this back
        //{"msgType":"ReadDeviceAck","actionResult":"device not exist"}
        //Hmmm, this is not really helpful. It doesn't contian the mac of the device which doesn't exist.
        //Tryig to remember which was the last mac enquired about could get messy
        //The only thing I can really do is do nothing
        Serial.printf("Skipping message");
        return mqttMessage::emptyMessage();
    }
     
    auto deviceMac = std::string((const char *)doc["mac"]);
    auto bidirectional = data["wirelessMode"] == 1;

    if(!bidirectional)
    {
        return mqttMessage::createUniDirectionalDeviceMessage(deviceMac.c_str(), updateType);
    }

    auto batteryLevel = (int) data["batteryLevel"];
    auto position = (int) data["currentPosition"];
    auto rssi = (int) data["RSSI"];
    auto shadeType = DeviceType::ShadeTypeName(data["type"]);

    return mqttMessage::createBiDirectionalDeviceMessage(deviceMac.c_str(), updateType, position, shadeType.c_str(), batteryLevel, rssi);
}

std::string ConnectorUdp::parseHubDetailsAndFindMac(JsonDocument& doc)
{
    auto token = (const char *)doc["token"];
    udpMessages.setHubToken(token);
    return std::string((const char *)doc["mac"]);
}



