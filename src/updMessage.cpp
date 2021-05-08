#include "udpMessage.h"


udpMessage::udpMessage(IPAddress sendIp, int port, const char* body)
    :sendIp(sendIp), port(port), body(body)
{}

bool udpMessage::send(WiFiUDP& udpClient)
{
    udpClient.beginPacket(sendIp, port);
    udpClient.write(body.c_str(), body.length());
    return udpClient.endPacket() > 0;
}