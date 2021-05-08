#ifndef UDP_MESSAGE_H
#define UDP_MESSAGE_H

#include <WiFiUDP.h>
#include <string>

class udpMessage
{
    public:
    udpMessage(IPAddress sendIp, int port, const char* body);

    bool send(WiFiUDP& udpClient);

    private:
    IPAddress sendIp;
    int port;
    std::string body;
};

#endif