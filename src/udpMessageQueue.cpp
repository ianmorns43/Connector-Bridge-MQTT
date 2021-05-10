#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "udpMessageQueue.h"
#include "deviceType.h"


udpMessageQueue::udpMessageQueue()
 : multicastIp(238,0,0,18), noMessage()
{
}

void udpMessageQueue::beginListening()
{
    timestamp.start();
    udpClient.beginMulticast(WiFi.localIP(), multicastIp, listenPort);
}

IPAddress udpMessageQueue::getUnicastIp()
{
    return unicastIp;
}

void udpMessageQueue::setHubToken(const char* token)
{
    accessToken.setToken(token);
}

void udpMessageQueue::queueUnicastMessage(const char* body)
{
    //TODO: we cant send unicast messages until we know the IP address of the Hub
    messageQueue.push(new udpMessage(unicastIp, sendPort, body));
}

void udpMessageQueue::queueMulticastMessage(const char* body)
{
    messageQueue.push(new udpMessage(multicastIp, sendPort, body));
}

bool udpMessageQueue::sendNextMessage()
{
    auto now = millis();
    
    if( (now + delayBetweenMessagesMS) < dontSendBeforeMillis)
    {
        //If millis() has wrapped then millis() << dontSendBeforeMillis. Deal with this by resetting broacast time to 100ms from now
        dontSendBeforeMillis = now + delayBetweenMessagesMS;
    }

    if(messageQueue.empty() || now < dontSendBeforeMillis)
    {
        return false;
    }
    dontSendBeforeMillis = millis() + delayBetweenMessagesMS;

    auto message = messageQueue.front();

    auto success = message->send(udpClient, timestamp.Generate());
    if(success)
    {
        messageQueue.pop();
        delete message;
    }

    return success;
}

const std::string& udpMessageQueue::readNextIncomingPacket()
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

void udpMessageQueue::queueMulticastDeviceListRequest()
{
    std::ostringstream stream;
    stream << "{ \"msgType\":\"GetDeviceList\",\"msgID\":\"" << udpMessage::TIMESTAMP_PLACEHOLDER << "\" }";  
    queueMulticastMessage(stream.str().c_str());
}

void udpMessageQueue::queueDeviceStatusRequest(const char* deviceMac)
{
    std::ostringstream stream;
    stream << "{" << basicDeviceMessage("ReadDevice", deviceMac) <<"}";
    queueUnicastMessage(stream.str().c_str());
}

void udpMessageQueue::queueSetPositionRequest(const char* key, const char* deviceMac, int newPosition)
{
    //Dooya: setting position 0 opens the blind and position 100 closes it
    //Alexa: setting position 100 opens the blind and position 0 closes it
    //We need to inver the position we are asking for so Dooya blind does what Alexa tells it to
    std::ostringstream stream;
    stream << "{" << deviceMessageWithAccessToken("WriteDevice", key, deviceMac) << ",\"data\":{\"targetPosition\":" << 100-newPosition << "}}";
    queueUnicastMessage(stream.str().c_str());
}

void udpMessageQueue::queueOpenRequest(const char* key, const char* deviceMac)
{
    queueDeviceOperationMessage(key, deviceMac, 1);
}

void udpMessageQueue::queueCloseRequest(const char* key, const char* deviceMac)
{
    queueDeviceOperationMessage(key, deviceMac, 0);
}

void udpMessageQueue::queueStopRequest(const char* key, const char* deviceMac)
{
    queueDeviceOperationMessage(key, deviceMac, 2);
}
    
void udpMessageQueue::queueDeviceOperationMessage(const char* key, const char* deviceMac, int operation)
{
    std::ostringstream stream;
    stream << "{" << deviceMessageWithAccessToken("WriteDevice", key, deviceMac) << ",\"data\":{\"operation\":" << operation << "}}";
    queueUnicastMessage(stream.str().c_str());
}

std::string udpMessageQueue::basicDeviceMessage(const char* msgType, const char* deviceMac)
{
    std::ostringstream stream;
    stream << "\"msgType\":\"" << msgType << "\",\"mac\":\"" << deviceMac << "\",\"deviceType\":\"" << DeviceType::RFMotor << "\",\"msgID\":\"" << udpMessage::TIMESTAMP_PLACEHOLDER << "\"";
    return stream.str();
}

std::string udpMessageQueue::deviceMessageWithAccessToken(const char* msgType, const char* key, const char* deviceMac)
{
    std::ostringstream stream;
    stream << basicDeviceMessage(msgType, deviceMac) << ",\"AccessToken\":\"" << accessToken.getAccessToken(key) << "\"";
    return stream.str();
}


