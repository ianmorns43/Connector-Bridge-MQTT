#include "mqttMessage.h"
#include <Arduino.h>

mqttMessage::mqttMessage(const char* topic, std::string& payload)
:topic(topic), payload(payload)
{}

bool mqttMessage::empty()
{
    return topic.empty() && payload.empty();
}

void mqttMessage::publish(PubSubClient& mqttClient)
{
    std::ostringstream stream;
    stream << MQTT_TOPIC << "/status/" << topic;
    auto fullTopic = stream.str().c_str();

    Serial.printf("Publish to: %s, %s\r\n", fullTopic, payload.c_str());
    mqttClient.publish(fullTopic, payload.c_str());
}

mqttMessage mqttMessage::emptyMessage()
{
    auto emptyString = std::string();
    return mqttMessage(emptyString.c_str(),emptyString);
}

mqttMessage mqttMessage::createHubMessage(int deviceCount, const char* hubMac)
{
    return createHubMessage(deviceCount, hubMac, 0);
}

mqttMessage mqttMessage::createHubMessage(int deviceCount, const char* hubMac, int rssi)
{
    std::ostringstream stream;
    stream << "{";
    stream << "\"deviceCount\":" << deviceCount << "";
    stream << ",\"mac\":\"" << hubMac << "\"";
    if(rssi != 0)
    {
        stream << ",\"rssi\":" << rssi << "";
    }
    stream << "}";

    const char* hubTopic = "hub";
    std::string hubPayload = stream.str();
    return mqttMessage(hubTopic, hubPayload);
}

mqttMessage mqttMessage::createBiDirectionalDeviceMessage(const char* deviceMac, const char* updateType, int position, const char* shadeType, int batteryLevel, int rssi)
{
    std::ostringstream stream;
    stream << "{";
    stream << "\"updateType\":\"" << updateType << "\"";
    stream << ",\"bidirectional\":true";
    stream << ",\"position\":" << 100 - position;
    stream << ",\"shadeType\":\"" << shadeType << "\"";
    stream << ",\"battery\":" << batteryLevel;
    stream << ",\"rssi\":" << rssi;
    stream << "}";

    std::string devicePayload = stream.str();
    return mqttMessage(deviceMac, devicePayload);
}

mqttMessage mqttMessage::createUniDirectionalDeviceMessage(const char* deviceMac, const char* updateType)
{
    std::ostringstream stream;
    stream << "{";
    stream << "\"updateType\":\"" << updateType << "\"";
    stream << ",\"bidirectional\":false";
    stream << "}";

    std::string devicePayload = stream.str();
    return mqttMessage(deviceMac, devicePayload);
}