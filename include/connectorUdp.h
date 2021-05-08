#ifndef CONNECTOR_UDP_H
#define CONNECTOR_UDP_H
#include <Arduino.h>
#include <string>
#include <vector>
#include "timeStampGenerator.h"
#include "accessTokenGenerator.h"
#include "udpMessageQueue.h"

class ConnectorUdp
{
    public:
    ConnectorUdp();

    void start();
    void loop();

    void sendMulticastDeviceListRequest();

    private:
    udpMessageQueue udpMessages;
    
    TimeStampGenerator timestamp;
    AccessTokenGenerator accessToken;

    std::string hubMac;

    bool deviceListReceived = false;
    unsigned int lastTimeDeviceListRequested = 0;

    std::string deviceListMsg();
    std::string deviceStatusRequestMsg(const char* deviceMac);
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