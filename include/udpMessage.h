#ifndef UDP_MESSAGE_H
#define UDP_MESSAGE_H

#include <WiFiUDP.h>
#include <string>
#include "IMessage.h"


class udpMessage : public IMessage
{
    public:

    void send();
    bool isComplete() const;  

    protected:
    virtual bool send(const std::string& body) = 0;
    udpMessage(const char* body);

    private:
    std::string body;
    int sendAttemptsRemaining;
};

class multicastUdpMessage : public udpMessage
{
    public:
    multicastUdpMessage(const char* body);

    protected:
    bool send(const std::string& body);
};

class unicastUdpMessage : public udpMessage
{
    public:
    unicastUdpMessage(const char* body);

    protected:
    bool send(const std::string& body);
};

#endif