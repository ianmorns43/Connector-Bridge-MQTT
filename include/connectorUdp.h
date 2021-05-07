#ifndef CONNECTOR_UDP_H
#define CONNECTOR_UDP_H
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <string>
#include <vector>
#include "timeStampGenerator.h"
#include "accessTokenGenerator.h"

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
    AccessTokenGenerator accessToken;

    std::vector<char> readBuffer;

    unsigned int hubResponsePort = 32101;  // port to listen on
    unsigned int sendPort = 32100;  // port to send on
    IPAddress multicastIP;

    IPAddress hubIp;
    std::string hubMac;

    std::string GetDeviceListMsg();
    /*if(!cmdSent)
    {
      const String posTo44 = "{\"msgType\":\"WriteDevice\",\"mac\":\"f008d1edd4ec0002\",\"deviceType\":\"10000000\",\"AccessToken\":\"***********\",\"msgID\":\"20210508144209916\",\"data\":{\"targetPosition\":44}}";
      multicastUpd.beginPacket(multicastIP, sendPort);
      multicastUpd.write(posTo44.c_str(), posTo44.length());
      multicastUpd.endPacket();
      cmdSent = true;
    }*/

    class DeviceType
    {
        public:

        static std::string RFMotor;
        static std::string WiFiCurtain;
        static std::string WiFiBridge;
        static std::string WiFiTubularMotor;
        static std::string WiFiReceiver;

        static std::string ShadeTypeName(int type);

        private:
        DeviceType(){}

        static std::array<std::string, 14> shadeTypes;
    };
};

#endif