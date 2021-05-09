#include "connectorUdp.h"
#include "ArduinoJson.h"
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

void ConnectorUdp::loop()
{
    auto packet = udpMessages.readNextIncomingPacket();
    
    if (!packet.empty())
    {       
        JsonDocumentRoot doc;
        auto error = deserializeJson(doc, packet);
        if(error)
        {
            Serial.printf("Error: %s\r\n", error.c_str());
        }

        auto messageType = std::string((const char *)doc["msgType"]);

        if(messageType == "Heartbeat")
        {
            auto token = (const char *)doc["token"];
            udpMessages.setHubToken(token);
            hubMac = std::string((const char *)doc["mac"]);
            auto rssi = (int) (doc["data"])["RSSI"];

            Serial.printf("Heartbeat: token = %s, mac = %s, HubIp = %s, RSSI = %d\r\n", token, hubMac.c_str(), udpMessages.getUnicastIp().toString().c_str(), rssi);
        
        }
        else if(messageType == "GetDeviceListAck")
        {
            deviceListReceived = true;
            auto token = (const char *)doc["token"];
            udpMessages.setHubToken(token);
            hubMac = std::string((const char *)doc["mac"]);
            
            auto data = doc["data"];

            std::vector<std::string> deviceList;
            deviceList.reserve(data.size());

            for(size_t i = 0; i < data.size(); i++)
            {
                auto device = data[i];
                auto deviceType = (const char*)device["deviceType"];

                if(DeviceType::RFMotor == deviceType)
                {
                    auto deviceMac = (const char*) device["mac"];
                    udpMessages.queueDeviceStatusRequest(deviceMac);
                    deviceList.push_back(deviceMac);
                }
            }

            Serial.printf("GetDeviceListAck: token = %s, mac = %s, HubIp = %s, Device count = %d\r\n", token, hubMac.c_str(), udpMessages.getUnicastIp().toString().c_str(), deviceList.size());
            Serial.print("MACs: ");
            for(auto it = deviceList.begin(); it != deviceList.end(); ++it)
            {
                Serial.printf(" %s", it->c_str());
            }
            Serial.println();

        }
        else if(messageType == "WriteDeviceAck")
        {
            auto deviceMac = std::string((const char *)doc["mac"]);
            auto data = doc["data"];

            auto operation = (int) data["operation"]; //Is this always 2?

            auto batteryLevel = (int) data["batteryLevel"];
            auto currentPosition = (int) data["currentPosition"];
            auto rssi = (int) data["RSSI"];
            auto shadeType = DeviceType::ShadeTypeName(data["type"]);
            auto isBidirectional = data["wirelessMode"] == 1;

            Serial.printf("WriteDeviceAck: operation %d(=2?) deviceMac = %s, batteryLevel = %d, currentPosition = %d, rssi = %d, type = %s, is Bi-directional %d\r\n",operation, deviceMac.c_str(), batteryLevel, currentPosition, rssi, shadeType.c_str(), isBidirectional);
        }
        else if(messageType == "Report") //When motor stops
        {
            auto deviceMac = std::string((const char *)doc["mac"]);
            auto data = doc["data"];

            auto operation = (int) data["operation"]; //Is this always 2?

            auto batteryLevel = (int) data["batteryLevel"];
            auto currentPosition = (int) data["currentPosition"];
            auto rssi = (int) data["RSSI"];
            auto shadeType = DeviceType::ShadeTypeName(data["type"]);
            auto isBidirectional = data["wirelessMode"] == 1;

            Serial.printf("Report: operation %d(=2?) deviceMac = %s, batteryLevel = %d, currentPosition = %d, rssi = %d, type = %s, is Bi-directional %d\r\n",operation, deviceMac.c_str(), batteryLevel, currentPosition, rssi, shadeType.c_str(), isBidirectional);
        }
        else if(messageType == "ReadDeviceAck") //When asked for
        {            
            auto deviceMac = std::string((const char *)doc["mac"]);
            auto data = doc["data"];

            auto operation = (int) data["operation"]; //Is this always 2?

            auto batteryLevel = (int) data["batteryLevel"];
            auto currentPosition = (int) data["currentPosition"];
            auto rssi = (int) data["RSSI"];
            auto shadeType = DeviceType::ShadeTypeName(data["type"]);
            auto isBidirectional = data["wirelessMode"] == 1;

            Serial.printf("ReadDeviceAck: operation %d(=2?) deviceMac = %s, batteryLevel = %d, currentPosition = %d, rssi = %d, type = %s, is Bi-directional %d\r\n",operation, deviceMac.c_str(), batteryLevel, currentPosition, rssi, shadeType.c_str(), isBidirectional);
        }
    }
    else if(!deviceListReceived && millis() > (lastTimeDeviceListRequested + 5000u))
    {
        lastTimeDeviceListRequested = millis();
        deviceListReceived = false;
        Serial.println("Asking hub to identify itself...");
        udpMessages.queueMulticastDeviceListRequest();
    }
    else
    {
        udpMessages.sendNextMessage();
    }
}



