#ifndef CONNECTOR_MQTT_CLIENT_H
#define CONNECTOR_MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "transmitQueue.h"

class ConnectorMqttClient
{
    public:

    static void setup(TransmitQueue& messageQueueRef);
    static void loop();

    static PubSubClient mqttClient;  

    private:

    static void mqttConnect();
    static void mqttCallback(std::string topic, byte* message, unsigned int length);

    static WiFiClient espClient;  

    static TransmitQueue* messageQueue;
};

#endif