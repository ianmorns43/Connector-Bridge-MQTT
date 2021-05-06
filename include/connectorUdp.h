#ifndef CONNECTOR_UDP_H
#define CONNECTOR_UDP_H
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "timeStampGenerator.h"
#include <string>
#include <sstream>

class ConnectorUdp
{
    private:
    WiFiUDP udpSocket;
    TimeStampGenerator timestamp;

    unsigned int hubResponsePort = 32101;  // port to listen on
    unsigned int sendPort = 32100;  // port to send on
    IPAddress multicastIP;

    std::string GetDeviceListMsg()
    {
        std::ostringstream stream;
        stream << "{ \"msgType\":\"GetDeviceList\",\"msgID\":\"" << timestamp.Generate() << "\" }";

        return stream.str();
    }

    public:
    ConnectorUdp()
    :multicastIP(238,0,0,18)
    {}

    void start()
    {
        timestamp.start();
        //Listen for broadcast messages
        udpSocket.beginMulticast(WiFi.localIP(), multicastIP, hubResponsePort);
    }

    void loop()
    {
        int packetSize = udpSocket.parsePacket();
        if (packetSize)
        {
            bool cmdSent = false;
            const unsigned int incomingPacketBufferSize = 1024;
            char incomingPacket[incomingPacketBufferSize];  // buffer for incoming packets

            Serial.printf("Received %d bytes from %s:%d, on %s:%d\n", packetSize, udpSocket.remoteIP().toString().c_str(), udpSocket.remotePort(), udpSocket.destinationIP().toString().c_str(), hubResponsePort);
            udpSocket.read(incomingPacket, incomingPacketBufferSize);
            Serial.println(incomingPacket);
        }
    }

    void sendMulticastDeviceListRequest()
    {
        auto getDeviceList = GetDeviceListMsg();

        udpSocket.beginPacket(multicastIP, sendPort);
        udpSocket.write(getDeviceList.c_str(), getDeviceList.length());
        udpSocket.endPacket();
    }

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