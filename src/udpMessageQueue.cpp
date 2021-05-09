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

void udpMessageQueue::queueDeviceStatusRequest(const char* deviceMac)
{
    queueUnicastMessage(deviceStatusRequestMsg(deviceMac).c_str());
}

void udpMessageQueue::queueMulticastDeviceListRequest()
{  
    queueMulticastMessage(deviceListMsg().c_str());
}

std::string udpMessageQueue::deviceListMsg()
{
    std::ostringstream stream;
    stream << "{ \"msgType\":\"GetDeviceList\",\"msgID\":\"" << udpMessage::TIMESTAMP_PLACEHOLDER << "\" }";

    return stream.str();
}

std::string udpMessageQueue::deviceStatusRequestMsg(const char* deviceMac)
{
    std::ostringstream stream;
    stream << "{ \"msgType\":\"ReadDevice\",\"mac\":\"" << deviceMac << "\",\"deviceType\":\"" << DeviceType::RFMotor << "\",\"msgID\":\"" << udpMessage::TIMESTAMP_PLACEHOLDER << "\" }";

    return stream.str();
}