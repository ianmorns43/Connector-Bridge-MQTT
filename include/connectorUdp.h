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

    /*if(!cmdSent)
    {
      const String posTo44 = "{\"msgType\":\"WriteDevice\",\"mac\":\"f008d1edd4ec0002\",\"deviceType\":\"10000000\",\"AccessToken\":\"***********\",\"msgID\":\"20210508144209916\",\"data\":{\"targetPosition\":44}}";
      multicastUpd.beginPacket(multicastIP, sendPort);
      multicastUpd.write(posTo44.c_str(), posTo44.length());
      multicastUpd.endPacket();
      cmdSent = true;
    }*/
};

#endif