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
    :multicastIP(238,0,0,18)
{
}

void ConnectorUdp::start()
{
    timestamp.start();
    //Listen for broadcast messages
    udpSocket.beginMulticast(WiFi.localIP(), multicastIP, hubResponsePort);
}

void ConnectorUdp::loop()
{
    int packetSize = udpSocket.parsePacket();
    
    if (packetSize)
    {
        Serial.printf("Received %d bytes\r\n", packetSize);
        messageBuffer.erase();
        messageBuffer.reserve(packetSize);

        for(auto i = 0; i < packetSize; i++)
        {
            messageBuffer.push_back(udpSocket.read());
        }

        Serial.printf("Read %d bytes from %s:%d, on %s:%d\n", messageBuffer.size(), udpSocket.remoteIP().toString().c_str(), udpSocket.remotePort(), udpSocket.destinationIP().toString().c_str(), hubResponsePort);
        //Serial.println(messageBuffer.c_str());
        
        JsonDocumentRoot doc;
        auto error = deserializeJson(doc, messageBuffer);
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
            hubIp = udpSocket.remoteIP();

            auto rssi = (int) (doc["data"])["RSSI"];

            Serial.printf("Heartbeat: token = %s, mac = %s, HubIp = %s, RSSI = %d\r\n", token.c_str(), hubMac.c_str(), hubIp.toString().c_str(), rssi);
        
        }
        else if(messageType == "GetDeviceListAck")
        {
            auto token = std::string((const char *)doc["token"]);
            accessToken.setToken(token.c_str());
            hubMac = std::string((const char *)doc["mac"]);
            hubIp = udpSocket.remoteIP();

            auto data = doc["data"];

            std::vector<std::string> deviceList;
            deviceList.reserve(data.size());

            for(size_t i = 0; i < data.size(); i++)
            {
                auto device = data[i];
                auto deviceType = (const char*)device["deviceType"];

                Serial.printf("Device type: %s\r\n", deviceType);
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

            Serial.printf("WriteDeviceAck: operation %d(=2?) deviceMac = %s, batteryLevel = %d, currentPosition = %d, rssi = %d, type = %s, is Bi-directional %d\r\n",operation, deviceMac.c_str(), batteryLevel, currentPosition, rssi, shadeType.c_str(), isBidirectional);
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
}

void ConnectorUdp::sendMulticastDeviceListRequest()
{
    auto getDeviceList = GetDeviceListMsg();
    Serial.println(getDeviceList.c_str());
    udpSocket.beginPacket(multicastIP, sendPort);
    udpSocket.write(getDeviceList.c_str(), getDeviceList.length());
    udpSocket.endPacket();
}

std::string ConnectorUdp::GetDeviceListMsg()
{
    std::ostringstream stream;
    stream << "{ \"msgType\":\"GetDeviceList\",\"msgID\":\"" << timestamp.Generate() << "\" }";

    return stream.str();
}



