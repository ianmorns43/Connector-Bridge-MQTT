#include <Arduino.h>
#include "udpMessage.h"


udpMessage::udpMessage(IPAddress sendIp, int port, const char* body)
    :sendIp(sendIp), port(port), body(body)
{}

bool udpMessage::send(WiFiUDP& udpClient)
{
    Serial.printf("To %s:%d %s\r\n", sendIp.toString().c_str(), port, body.c_str());
    udpClient.beginPacket(sendIp, port);
    udpClient.write(body.c_str(), body.length());
    return udpClient.endPacket() > 0;
}