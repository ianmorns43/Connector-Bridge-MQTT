#ifndef MQTT_MESSAGE_H
#define MQTT_MESSAGE_H

#include <string>
#include <sstream>
#include <PubSubClient.h>

class mqttMessage
{ 
    public:
    bool empty();
    void publish(PubSubClient& mqttClient);
    static mqttMessage emptyMessage();
    static mqttMessage createHubMessage(int deviceCount, const char* hubMac);
    static mqttMessage createHubMessage(int deviceCount, const char* hubMac, int rssi);
    static mqttMessage createBiDirectionalDeviceMessage(const char* deviceMac, const char* updateType, int position, const char* shadeType, int batteryLevel, int rssi);
    static mqttMessage createUniDirectionalDeviceMessage(const char* deviceMac, const char* updateType);

    private:
    mqttMessage(const char* topic, std::string& payload);
    std::string topic;
    std::string payload;

};

#endif