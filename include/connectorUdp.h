#ifndef CONNECTOR_UDP_H
#define CONNECTOR_UDP_H
#include <Arduino.h>
#include <string>
#include <vector>
#include "ArduinoJson.h"
#include "transmitQueue.h"
#include "mqttMessage.h"

class ConnectorUdp
{
    public:
    ConnectorUdp(TransmitQueue& messageQueue);

    void start();
    mqttMessage loop();

    private:
    std::string parseHubDetailsAndFindMac(JsonDocument& doc);
    mqttMessage createDeviceMessage(const char* updateType, JsonDocument& doc);
    TransmitQueue& messageQueue;

    bool deviceListReceived = false;
    unsigned int lastTimeDeviceListRequested = 0;
};

#endif