#ifndef UDP_MESSAGE_QUEUE_H
#define UDP_MESSAGE_QUEUE_H

#include <WiFiUdp.h>
#include <queue>
#include <string>
#include "udpMessage.h"
#include "timeStampGenerator.h"
#include "accessTokenGenerator.h"

class udpMessageQueue
{
    public:
    udpMessageQueue();

    void beginListening();

    const std::string& readNextIncomingPacket();
    bool sendNextMessage();


    void queueMulticastDeviceListRequest();
    void queueDeviceStatusRequest(const char* deviceMac);
    void queueSetPositionRequest(const char* key, const char* deviceMac, int newPosition);
    void queueOpenRequest(const char* key, const char* deviceMac);
    void queueCloseRequest(const char* key, const char* deviceMac);
    void queueStopRequest(const char* key, const char* deviceMac);

    IPAddress getUnicastIp();
    void setHubToken(const char* token);

    private:

    void queueUnicastMessage(const char* body);
    void queueMulticastMessage(const char* body);
    void queueDeviceOperationMessage(const char* key, const char* deviceMac, int operation);
    std::string basicDeviceMessage(const char* msgType, const char* deviceMac);
    std::string deviceMessageWithAccessToken(const char* msgType, const char* key, const char* deviceMac);

    TimeStampGenerator timestamp;
    AccessTokenGenerator accessToken;

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