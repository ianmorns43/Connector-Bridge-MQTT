#ifndef CONNECTOR_UDP_H
#define CONNECTOR_UDP_H
#include <Arduino.h>
#include <string>
#include <vector>
#include "udpMessageQueue.h"

class ConnectorUdp
{
    public:
    ConnectorUdp(udpMessageQueue& udpMessages);

    void start();
    void loop();

    private:
    udpMessageQueue& udpMessages;
    std::string hubMac;

    bool deviceListReceived = false;
    unsigned int lastTimeDeviceListRequested = 0;
};

#endif