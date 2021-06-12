#include "connectorUdp.h"
#include <ESP8266WiFi.h>
#include "apiCodes.h"
#include "flasher.h"
#include "apiCodes.h"
#include "udpMessage.h"


TimeStampGenerator ConnectorUdp::timestamp;
AccessTokenGenerator ConnectorUdp::accessToken;

WiFiUDP ConnectorUdp::udpClient;
IPAddress ConnectorUdp::unicastIp;
std::string ConnectorUdp::messageBuffer;
const IPAddress ConnectorUdp::multicastIp(238,0,0,18);
const unsigned int ConnectorUdp::listenPort = 32101;  // port to listen on
const unsigned int ConnectorUdp::sendPort = 32100;  // port to send on
const std::string ConnectorUdp::noMessage = "";
const std::string ConnectorUdp::TIMESTAMP_PLACEHOLDER = "____TIMESTAMP____";

typedef StaticJsonDocument<2048> JsonDocumentRoot;

ConnectorUdp::ConnectorUdp(TransmitQueue& messageQueue)
:messageQueue(messageQueue)
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
    auto packet = readNextIncomingPacket();
    
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
            flasher::switchOff();

            auto hubMac = parseHubDetailsAndFindMac(doc);
            auto data = doc["data"];
 
            mqttMessage::DeviceList deviceList;
            deviceList.reserve(data.size());

            for(size_t i = 0; i < data.size(); i++)
            {
                auto device = data[i];
                auto deviceType = (const char*)device["deviceType"];

                if(ApiCodes::RFMotor == deviceType)
                {
                    auto deviceMac = (const char*) device["mac"];
                    deviceList.push_back(deviceMac);
                }
            }

            return mqttMessage::createDeviceListMessage(hubMac.c_str(), deviceList);

        }
        else if(messageType == "WriteDeviceAck")
        {
            //Serial.printf("Moving: %s\r\n", packet.c_str());
            return createDeviceMessage("moving", doc);            
        }
        else if(messageType == "Report") //When motor stops
        {
            //Serial.printf("Moved: %s\r\n", packet.c_str());
            return createDeviceMessage("moved", doc);
        }
        else if(messageType == "ReadDeviceAck") //When asked for
        {
            //Serial.printf("Status: %s\r\n", packet.c_str());           
            return createDeviceMessage("status", doc);
        }

        Serial.printf("Unrecognised udp: %s\r\n", packet.c_str());
    }
    else if(!deviceListReceived && millis() > (lastTimeDeviceListRequested + 5000u))
    {
        lastTimeDeviceListRequested = millis();
        deviceListReceived = false;
        Serial.println("Asking hub to identify itself...");
        messageQueue.enqueue(createMulticastDeviceListRequest());
    }

    if(!deviceListReceived)
    {
        flasher::flash(250u);
    }

    return mqttMessage::emptyMessage();
}

const std::string& ConnectorUdp::readNextIncomingPacket()
{
    int packetSize = udpClient.parsePacket();
    if(packetSize == 0)
    {
        return noMessage;
    }
    
    Serial.printf("Received %d bytes\r\n", packetSize);
    messageBuffer.erase();
    messageBuffer.reserve(packetSize);

    for(auto i = 0; i < packetSize; i++)
    {
        messageBuffer.push_back(udpClient.read());
    }

    Serial.printf("Read %d bytes from %s:%d, on %s:%d\n", messageBuffer.size(), udpClient.remoteIP().toString().c_str(), udpClient.remotePort(), udpClient.destinationIP().toString().c_str(), listenPort);
    //Serial.println(messageBuffer.c_str());

    if(udpClient.remoteIP() != multicastIp)
    {
        unicastIp = udpClient.remoteIP();
    }

    return messageBuffer;
}

bool ConnectorUdp::sendUnicast(const std::string& body)
{
    return send(unicastIp, body);
}

bool ConnectorUdp::sendMulticast(const std::string& body)
{
    return send(multicastIp, body);
}

bool ConnectorUdp::send(IPAddress sendIp, const std::string& body)
{
    auto tempBody = body;
    auto index = tempBody.find(TIMESTAMP_PLACEHOLDER);
    if(index != std::string::npos)
    {
        auto timestampCode = timestamp.Generate();
        tempBody.replace(index, timestampCode.size(), timestampCode);
    }

    Serial.printf("To %s:%d %s\r\n", sendIp.toString().c_str(), sendPort, tempBody.c_str());
    udpClient.beginPacket(sendIp, sendPort);
    udpClient.write(tempBody.c_str(), tempBody.length());
    return udpClient.endPacket() != 0;
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
    auto shadeType = ApiCodes::ShadeTypeName(data["type"]);
    

    if(!bidirectional)
    {
        auto operation = ApiCodes::OperationName(data["operation"]);
        //TODO add shadeType and operation to unidirectional blind
        return mqttMessage::createUniDirectionalDeviceMessage(deviceMac.c_str(), updateType, shadeType.c_str(), operation.c_str());
    }

    auto batteryLevel = (int) data["batteryLevel"];
    auto position = (int) data["currentPosition"];
    auto rssi = (int) data["RSSI"];
    

    return mqttMessage::createBiDirectionalDeviceMessage(deviceMac.c_str(), updateType, position, shadeType.c_str(), batteryLevel, rssi);
}

std::string ConnectorUdp::parseHubDetailsAndFindMac(JsonDocument& doc)
{
    auto token = (const char *)doc["token"];
    accessToken.setToken(token);
    return std::string((const char *)doc["mac"]);
}


void ConnectorUdp::beginListening()
{
    timestamp.start();
    udpClient.beginMulticast(WiFi.localIP(), multicastIp, listenPort);
}

IMessage* ConnectorUdp::createUnicastMessage(const char* body)
{
    //TODO: we cant send unicast messages until we know the IP address of the Hub
    return new unicastUdpMessage(body);
}

IMessage* ConnectorUdp::createMulticastMessage(const char* body)
{
    return new multicastUdpMessage(body);
}

IMessage* ConnectorUdp::createMulticastDeviceListRequest()
{
    std::ostringstream stream;
    stream << "{ \"msgType\":\"GetDeviceList\",\"msgID\":\"" << TIMESTAMP_PLACEHOLDER << "\" }";  
    return createMulticastMessage(stream.str().c_str());
}

IMessage* ConnectorUdp::createDeviceStatusRequest(const char* deviceMac)
{
    std::ostringstream stream;
    stream << "{" << basicDeviceMessage("ReadDevice", deviceMac) <<"}";
    return createUnicastMessage(stream.str().c_str());
}

IMessage* ConnectorUdp::createSetPositionRequest(const char* key, const char* deviceMac, int newPosition)
{
    //Dooya: setting position 0 opens the blind and position 100 closes it
    //Alexa: setting position 100 opens the blind and position 0 closes it
    //We need to inver the position we are asking for so Dooya blind does what Alexa tells it to
    std::ostringstream stream;
    stream << "{" << deviceMessageWithAccessToken("WriteDevice", key, deviceMac) << ",\"data\":{\"targetPosition\":" << 100-newPosition << "}}";
    return createUnicastMessage(stream.str().c_str());
}

IMessage* ConnectorUdp::createOpenRequest(const char* key, const char* deviceMac)
{
    return createDeviceOperationMessage(key, deviceMac, 1);
}

IMessage* ConnectorUdp::createCloseRequest(const char* key, const char* deviceMac)
{
    return createDeviceOperationMessage(key, deviceMac, 0);
}

IMessage* ConnectorUdp::createStopRequest(const char* key, const char* deviceMac)
{
    return createDeviceOperationMessage(key, deviceMac, 2);
}
    
IMessage* ConnectorUdp::createDeviceOperationMessage(const char* key, const char* deviceMac, int operation)
{
    std::ostringstream stream;
    stream << "{" << deviceMessageWithAccessToken("WriteDevice", key, deviceMac) << ",\"data\":{\"operation\":" << operation << "}}";
    return createUnicastMessage(stream.str().c_str());
}

std::string ConnectorUdp::basicDeviceMessage(const char* msgType, const char* deviceMac)
{
    std::ostringstream stream;
    stream << "\"msgType\":\"" << msgType << "\",\"mac\":\"" << deviceMac << "\",\"deviceType\":\"" << ApiCodes::RFMotor << "\",\"msgID\":\"" << TIMESTAMP_PLACEHOLDER << "\"";
    return stream.str();
}

std::string ConnectorUdp::deviceMessageWithAccessToken(const char* msgType, const char* key, const char* deviceMac)
{
    std::ostringstream stream;
    stream << basicDeviceMessage(msgType, deviceMac) << ",\"AccessToken\":\"" << accessToken.getAccessToken(key) << "\"";
    return stream.str();
}

