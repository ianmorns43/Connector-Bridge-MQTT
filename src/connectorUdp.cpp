#include "connectorUdp.h"
#include "ArduinoJson.h"

typedef StaticJsonDocument<4048> JsonDocumentRoot;

ConnectorUdp::ConnectorUdp()
    :multicastIP(238,0,0,18)
{
}

void ConnectorUdp::start()
{
    timestamp.start();
    //Listen for broadcast messages
    udpSocket.beginMulticast(WiFi.localIP(), multicastIP, hubResponsePort);
}

void ConnectorUdp::loop()
{
    int packetSize = udpSocket.parsePacket();
    if (packetSize)
    {
        readBuffer.resize(packetSize);
        
        Serial.printf("Received %d bytes from %s:%d, on %s:%d\n", packetSize, udpSocket.remoteIP().toString().c_str(), udpSocket.remotePort(), udpSocket.destinationIP().toString().c_str(), hubResponsePort);
        udpSocket.read(readBuffer.data(), readBuffer.size());

        auto message = std::string(readBuffer.data(), readBuffer.size());
        
        JsonDocumentRoot doc;
        /*auto error = */deserializeJson(doc, message);
        //TODO if(error)

        auto messageType = std::string((const char *)doc["msgType"]);

        if(messageType == "Heartbeat")
        {
            //Read the token
            //Record the IP
            //Record the mac
            //Signal the RSSI?
        }
        else if(messageType == "GetDeviceListAck")
        {
            //Read the token
            //Record the IP
            //Record the mac
            //Read the mac of devices with type 10000000
        }
        else if(messageType == "WriteDeviceAck")
        {
            //Read mac
            //Read battery level
            //Read current position?
            //Signal the RSSI?
        }
        else if(messageType == "Report") //When motor stops
        {
            //Signal: mac, type, currentPosition, battertLevel, wirelessMode?, RSSI
        }
        else if(messageType == "ReadDeviceAck") //When asked for
        {
            //Signal: mac, deviceType, type, currentPosition, battertLevel, wirelessMode?, RSSI
        }

        
    }
}

void ConnectorUdp::sendMulticastDeviceListRequest()
{
    auto getDeviceList = GetDeviceListMsg();

    udpSocket.beginPacket(multicastIP, sendPort);
    udpSocket.write(getDeviceList.c_str(), getDeviceList.length());
    udpSocket.endPacket();
}

std::string ConnectorUdp::GetDeviceListMsg()
{
    std::ostringstream stream;
    stream << "{ \"msgType\":\"GetDeviceList\",\"msgID\":\"" << timestamp.Generate() << "\" }";

    return stream.str();
}



