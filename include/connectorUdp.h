#ifndef CONNECTOR_UDP_H
#define CONNECTOR_UDP_H
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "timeStampGenerator.h"
#include <string>
#include <sstream>
#include <vector>

class ConnectorUdp
{
    public:
    ConnectorUdp();

    void start();
    void loop();

    void sendMulticastDeviceListRequest();

    private:
    WiFiUDP udpSocket;
    TimeStampGenerator timestamp;
    std::vector<char> readBuffer;

    unsigned int hubResponsePort = 32101;  // port to listen on
    unsigned int sendPort = 32100;  // port to send on
    IPAddress multicastIP;

    std::string GetDeviceListMsg();
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