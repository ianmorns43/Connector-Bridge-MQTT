#ifndef CONNECTOR_MQTT_CLIENT_H
#define CONNECTOR_MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "udpMessageQueue.h"

class ConnectorMqttClient
{
    public:

    static void setup(udpMessageQueue& messageQueueRef);
    static void loop();    

    private:
    static void publishStatus(const char* topic, const char* payload);
    static void mqttConnect();
    static void mqttCallback(std::string topic, byte* message, unsigned int length);

    static WiFiClient espClient;
    static PubSubClient mqttClient;

    static udpMessageQueue* messageQueue;
};

#endif