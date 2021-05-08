#ifndef UDP_MESSAGE_QUEUE_H
#define UDP_MESSAGE_QUEUE_H

#include <WiFiUdp.h>
#include <queue>
#include <string>
#include "udpMessage.h"

class udpMessageQueue
{
    public:
    udpMessageQueue();

    void beginListening();

    const std::string& readNextIncomingPacket();

    void queueUnicastMessage(const char* body);
    void queueMulticastMessage(const char* body);
    bool sendNextMessage();

    private:

    //The Dooya Connector Bridge documentation recommends leaving at least 100ms between messages sent to the bridge
    const int delayBetweenMessagesMS = 500;

    WiFiUDP udpClient;
    IPAddress unicastIp;
    IPAddress multicastIp;
    const unsigned int listenPort = 32101;  // port to listen on
    const unsigned int sendPort = 32100;  // port to send on
    
    std::string messageBuffer;
    const std::string noMessage;

    std::queue<udpMessage*> messageQueue;
    unsigned int dontSendBeforeMillis = 0;

};

#endif