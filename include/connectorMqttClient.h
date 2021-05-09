#ifndef CONNECTOR_MQTT_CLIENT_H
#define CONNECTOR_MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

class ConnectorMqttClient
{
    public:
    //Topics subscibed to
    static void setup();
    static void loop();    

    private:
    static void publishStatus(const char* topic, const char* payload);
    static void mqttConnect();
    static void mqttCallback(std::string topic, byte* message, unsigned int length);

    static WiFiClient espClient;
    static PubSubClient mqttClient;
};

#endif