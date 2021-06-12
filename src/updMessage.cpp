#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "udpMessage.h"
#include "apiCodes.h"


TimeStampGenerator udpMessage::timestamp;
AccessTokenGenerator udpMessage::accessToken;

WiFiUDP udpMessage::udpClient;
IPAddress udpMessage::unicastIp;
std::string udpMessage::messageBuffer;
const IPAddress udpMessage::multicastIp(238,0,0,18);
const unsigned int udpMessage::listenPort = 32101;  // port to listen on
const unsigned int udpMessage::sendPort = 32100;  // port to send on
const std::string udpMessage::noMessage = "";
const std::string udpMessage::TIMESTAMP_PLACEHOLDER = "____TIMESTAMP____";


udpMessage::udpMessage(IPAddress sendIp, const char* body)
    :sendIp(sendIp), body(body), sendAttemptsRemaining(3)
{}

void udpMessage::beginListening()
{
    timestamp.start();
    udpClient.beginMulticast(WiFi.localIP(), multicastIp, listenPort);
}

const std::string& udpMessage::readNextIncomingPacket()
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

void udpMessage::setHubToken(const char* token)
{
    accessToken.setToken(token);
}

void udpMessage::send()
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
    bool success = udpClient.endPacket() != 0;

    sendAttemptsRemaining = success ? 0 : sendAttemptsRemaining-1;
}

bool udpMessage::isComplete() const
{
    return sendAttemptsRemaining <= 0;
}

udpMessage* udpMessage::createUnicastMessage(const char* body)
{
    //TODO: we cant send unicast messages until we know the IP address of the Hub
    return new udpMessage(unicastIp, body);
}

udpMessage* udpMessage::createMulticastMessage(const char* body)
{
    return new udpMessage(multicastIp, body);
}

udpMessage* udpMessage::createMulticastDeviceListRequest()
{
    std::ostringstream stream;
    stream << "{ \"msgType\":\"GetDeviceList\",\"msgID\":\"" << udpMessage::TIMESTAMP_PLACEHOLDER << "\" }";  
    return createMulticastMessage(stream.str().c_str());
}

udpMessage* udpMessage::createDeviceStatusRequest(const char* deviceMac)
{
    std::ostringstream stream;
    stream << "{" << basicDeviceMessage("ReadDevice", deviceMac) <<"}";
    return createUnicastMessage(stream.str().c_str());
}

udpMessage* udpMessage::createSetPositionRequest(const char* key, const char* deviceMac, int newPosition)
{
    //Dooya: setting position 0 opens the blind and position 100 closes it
    //Alexa: setting position 100 opens the blind and position 0 closes it
    //We need to inver the position we are asking for so Dooya blind does what Alexa tells it to
    std::ostringstream stream;
    stream << "{" << deviceMessageWithAccessToken("WriteDevice", key, deviceMac) << ",\"data\":{\"targetPosition\":" << 100-newPosition << "}}";
    return createUnicastMessage(stream.str().c_str());
}

udpMessage* udpMessage::createOpenRequest(const char* key, const char* deviceMac)
{
    return createDeviceOperationMessage(key, deviceMac, 1);
}

udpMessage* udpMessage::createCloseRequest(const char* key, const char* deviceMac)
{
    return createDeviceOperationMessage(key, deviceMac, 0);
}

udpMessage* udpMessage::createStopRequest(const char* key, const char* deviceMac)
{
    return createDeviceOperationMessage(key, deviceMac, 2);
}
    
udpMessage* udpMessage::createDeviceOperationMessage(const char* key, const char* deviceMac, int operation)
{
    std::ostringstream stream;
    stream << "{" << deviceMessageWithAccessToken("WriteDevice", key, deviceMac) << ",\"data\":{\"operation\":" << operation << "}}";
    return createUnicastMessage(stream.str().c_str());
}

std::string udpMessage::basicDeviceMessage(const char* msgType, const char* deviceMac)
{
    std::ostringstream stream;
    stream << "\"msgType\":\"" << msgType << "\",\"mac\":\"" << deviceMac << "\",\"deviceType\":\"" << ApiCodes::RFMotor << "\",\"msgID\":\"" << TIMESTAMP_PLACEHOLDER << "\"";
    return stream.str();
}

std::string udpMessage::deviceMessageWithAccessToken(const char* msgType, const char* key, const char* deviceMac)
{
    std::ostringstream stream;
    stream << basicDeviceMessage(msgType, deviceMac) << ",\"AccessToken\":\"" << accessToken.getAccessToken(key) << "\"";
    return stream.str();
}
