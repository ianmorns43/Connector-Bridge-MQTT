#ifndef CONNECTOR_UDP_H
#define CONNECTOR_UDP_H
#include <Arduino.h>
#include <string>
#include <vector>
#include "ArduinoJson.h"
#include "udpMessageQueue.h"
#include "mqttMessage.h"

class ConnectorUdp
{
    public:
    ConnectorUdp(udpMessageQueue& udpMessages);

    void start();
    mqttMessage loop();

    private:
    std::string parseHubDetailsAndFindMac(JsonDocument& doc);
    mqttMessage createDeviceMessage(const char* updateType, JsonDocument& doc);
    udpMessageQueue& udpMessages;

    bool deviceListReceived = false;
    unsigned int lastTimeDeviceListRequested = 0;
};

#endif