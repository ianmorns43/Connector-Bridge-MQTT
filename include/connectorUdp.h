#ifndef CONNECTOR_UDP_H
#define CONNECTOR_UDP_H
#include <Arduino.h>
#include <string>
#include <vector>
#include "ArduinoJson.h"
#include "transmitQueue.h"
#include "mqttMessage.h"
#include "timeStampGenerator.h"
#include "accessTokenGenerator.h"
#include "IMessage.h"

class ConnectorUdp
{
    public:
    ConnectorUdp(TransmitQueue& messageQueue);

    void start();
    mqttMessage loop();

    static bool sendUnicast(const std::string& body);
    static bool sendMulticast(const std::string& body);

    static void beginListening();
    static IMessage* createMulticastDeviceListRequest();
    static IMessage* createDeviceStatusRequest(const char* deviceMac);
    static IMessage* createSetPositionRequest(const char* key, const char* deviceMac, int newPosition);
    static IMessage* createOpenRequest(const char* key, const char* deviceMac);
    static IMessage* createCloseRequest(const char* key, const char* deviceMac);
    static IMessage* createStopRequest(const char* key, const char* deviceMac);

    private:

    static const std::string& readNextIncomingPacket();
    
    static bool send(IPAddress sendIp, const std::string& body);
    std::string parseHubDetailsAndFindMac(JsonDocument& doc);
    mqttMessage createDeviceMessage(const char* updateType, JsonDocument& doc);

    static IMessage* createUnicastMessage(const char* body);
    static IMessage* createMulticastMessage(const char* body);


    static IMessage* createDeviceOperationMessage(const char* key, const char* deviceMac, int operation);
    static std::string basicDeviceMessage(const char* msgType, const char* deviceMac);
    static std::string deviceMessageWithAccessToken(const char* msgType, const char* key, const char* deviceMac);

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

    TransmitQueue& messageQueue;

    bool deviceListReceived = false;
    unsigned int lastTimeDeviceListRequested = 0;
};

#endif