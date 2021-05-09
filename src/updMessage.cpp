#include <Arduino.h>
#include "udpMessage.h"


std::string udpMessage::TIMESTAMP_PLACEHOLDER = "____TIMESTAMP____";

udpMessage::udpMessage(IPAddress sendIp, int port, const char* body)
    :sendIp(sendIp), port(port), body(body)
{}

bool udpMessage::send(WiFiUDP& udpClient, std::string timestamp)
{
    auto tempBody = body;
    auto index = tempBody.find(TIMESTAMP_PLACEHOLDER);
    if(index != std::string::npos)
    {
        tempBody.replace(index, timestamp.size(), timestamp);
    }

    Serial.printf("To %s:%d %s\r\n", sendIp.toString().c_str(), port, tempBody.c_str());
    udpClient.beginPacket(sendIp, port);
    udpClient.write(tempBody.c_str(), tempBody.length());
    return udpClient.endPacket() > 0;
}