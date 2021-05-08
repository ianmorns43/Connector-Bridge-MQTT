#include "connectorUdp.h"
#include "ArduinoJson.h"

//The code returned for different device types in Device List
std::string ConnectorUdp::DeviceType::RFMotor = "10000000";
std::string ConnectorUdp::DeviceType::WiFiCurtain = "22000000";
std::string ConnectorUdp::DeviceType::WiFiBridge = "02000001";
std::string ConnectorUdp::DeviceType::WiFiTubularMotor = "22000002";
std::string ConnectorUdp::DeviceType::WiFiReceiver = "22000005";

std::string ConnectorUdp::DeviceType::ShadeTypeName(int type)
{
    return shadeTypes[type-1];
}

std::array<std::string, 14> ConnectorUdp::DeviceType::shadeTypes = {
    "Roller Blinds",
    "Venetian Blinds",
    "Roman Blinds",
    "Honeycomb Blinds",
    "Shangri-La Blinds",
    "Roller Shutter",
    "Roller Gate",
    "Awning",
    "TDBU",
    "Day&night Blinds",
    "Dimming Blinds",
    "Curtain",
    "Curtain(Open Left)",
    "Curtain(Open Right)"
};

typedef StaticJsonDocument<2048> JsonDocumentRoot;

ConnectorUdp::ConnectorUdp()
{
}

void ConnectorUdp::start()
{
    timestamp.start();
    //Listen for broadcast messages
    udpMessages.beginListening();
    
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
            auto token = std::string((const char *)doc["token"]);
            accessToken.setToken(token.c_str());
            hubMac = std::string((const char *)doc["mac"]);
            auto rssi = (int) (doc["data"])["RSSI"];

            Serial.printf("Heartbeat: token = %s, mac = %s, HubIp = %s, RSSI = %d\r\n", token.c_str(), hubMac.c_str(), hubIp.toString().c_str(), rssi);
        
        }
        else if(messageType == "GetDeviceListAck")
        {
            deviceListReceived = true;
            auto token = std::string((const char *)doc["token"]);
            accessToken.setToken(token.c_str());
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
                    deviceList.push_back(deviceMac);
                }
            }

            Serial.printf("GetDeviceListAck: token = %s, mac = %s, HubIp = %s, Device count = %d\r\n", token.c_str(), hubMac.c_str(), hubIp.toString().c_str(), deviceList.size());
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
        Serial.println("Asking hub to identify itself...");
        sendMulticastDeviceListRequest();
    }
    else
    {
        udpMessages.sendNextMessage();
    }
}

void ConnectorUdp::sendMulticastDeviceListRequest()
{
    lastTimeDeviceListRequested = millis();
    deviceListReceived = false;
   
    udpMessages.queueMulticastMessage(GetDeviceListMsg().c_str());
}

std::string ConnectorUdp::GetDeviceListMsg()
{
    std::ostringstream stream;
    stream << "{ \"msgType\":\"GetDeviceList\",\"msgID\":\"" << timestamp.Generate() << "\" }";

    return stream.str();
}



