#ifndef UDP_MESSAGE_H
#define UDP_MESSAGE_H

#include <WiFiUDP.h>
#include <string>
#include "IMessage.h"
#include "timeStampGenerator.h"
#include "accessTokenGenerator.h"

class udpMessage : public IMessage
{
    public:

    void send();
    bool isComplete() const;  
    
    static void beginListening();
    static const std::string& readNextIncomingPacket();
    static void setHubToken(const char* token);

    static udpMessage* createMulticastDeviceListRequest();
    static udpMessage* createDeviceStatusRequest(const char* deviceMac);
    static udpMessage* createSetPositionRequest(const char* key, const char* deviceMac, int newPosition);
    static udpMessage* createOpenRequest(const char* key, const char* deviceMac);
    static udpMessage* createCloseRequest(const char* key, const char* deviceMac);
    static udpMessage* createStopRequest(const char* key, const char* deviceMac);

    private:
    udpMessage(IPAddress sendIp, const char* body);
    static udpMessage* createUnicastMessage(const char* body);
    static udpMessage* createMulticastMessage(const char* body);
    static udpMessage* createDeviceOperationMessage(const char* key, const char* deviceMac, int operation);
    static std::string basicDeviceMessage(const char* msgType, const char* deviceMac);
    static std::string deviceMessageWithAccessToken(const char* msgType, const char* key, const char* deviceMac);

    IPAddress sendIp;
    std::string body;
    int sendAttemptsRemaining;

    static TimeStampGenerator timestamp;
    static AccessTokenGenerator accessToken;

    static WiFiUDP udpClient;
    static IPAddress unicastIp;
    static std::string messageBuffer;
    
    const static IPAddress multicastIp;
    const static unsigned int listenPort;  // port to listen on
    const static unsigned int sendPort;  // port to send on
    const static std::string noMessage;
    const static std::string TIMESTAMP_PLACEHOLDER;
};

#endif