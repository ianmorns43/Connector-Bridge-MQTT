#include "mqttMessage.h"
#include <Arduino.h>
#include "flasher.h"

mqttMessage::mqttMessage(const char* topic, std::string& payload)
:topic(topic), payload(payload)
{}

bool mqttMessage::empty()
{
    return topic.empty() && payload.empty();
}

void mqttMessage::publish(PubSubClient& mqttClient)
{
    flasher::blinkOn(50);
    std::ostringstream stream;
    stream << MQTT_TOPIC << "/" << topic;
    auto fullTopic = stream.str().c_str();

    Serial.printf("Publish to: %s, %s\r\n", fullTopic, payload.c_str());
    mqttClient.publish(fullTopic, payload.c_str());
}

mqttMessage mqttMessage::emptyMessage()
{
    auto emptyString = std::string();
    return mqttMessage(emptyString.c_str(),emptyString);
}

mqttMessage mqttMessage::createDeviceListMessage(const char* hubMac, const DeviceList& deviceList)
{
    // {mac:"<hubMac>", "deviceMacs":[<device1Mac>",..., <deviceNMac>"]}
    std::ostringstream stream;
    stream << "{";
    stream << "\"mac\":\"" << hubMac << "\"";
    stream << ",\"deviceMacs\":[";

    auto it = deviceList.begin(); 
    while(it != deviceList.end())
    {
        stream << "\"" << *it << "\"";

        if( ++it != deviceList.end())
        {
            stream << ",";
        }
    }

    stream << "]}";

    const char* hubTopic = "hub/deviceList";
    std::string hubPayload = stream.str();
    return mqttMessage(hubTopic, hubPayload);
}

mqttMessage mqttMessage::createHeartbeatMessage(int deviceCount, const char* hubMac, int rssi)
{
    std::ostringstream stream;
    stream << "{";
    stream << "\"deviceCount\":" << deviceCount << "";
    stream << ",\"mac\":\"" << hubMac << "\"";
    stream << ",\"rssi\":" << rssi << "";
    stream << "}";

    const char* hubTopic = "hub/heartbeat";
    std::string hubPayload = stream.str();
    return mqttMessage(hubTopic, hubPayload);
}

mqttMessage mqttMessage::createBiDirectionalDeviceMessage(const char* deviceMac, const char* updateType, int position, const char* shadeType, int batteryLevel, int rssi)
{
    std::ostringstream payloadStream;
    payloadStream << "{";
    payloadStream << "\"mac\":\"" << deviceMac << "\"";
    payloadStream << ",\"bidirectional\":true";
    payloadStream << ",\"position\":" << 100 - position;
    payloadStream << ",\"shadeType\":\"" << shadeType << "\"";
    payloadStream << ",\"battery\":" << batteryLevel;
    payloadStream << ",\"rssi\":" << rssi;
    payloadStream << "}";

    std::ostringstream topicStream;
    topicStream << "blind/" << updateType;

    std::string devicePayload = payloadStream.str();
    return mqttMessage(topicStream.str().c_str(), devicePayload);
}

mqttMessage mqttMessage::createUniDirectionalDeviceMessage(const char* deviceMac, const char* updateType, const char* shadeType, const char* operation)
{
    std::ostringstream payloadStream;
    payloadStream << "{";
    payloadStream << "\"mac\":\"" << deviceMac << "\"";
    payloadStream << ",\"bidirectional\":false";
    payloadStream << ",\"shadeType\":\"" << shadeType << "\"";
    payloadStream << ",\"operation\":\"" << operation << "\"";
    payloadStream << "}";

    std::ostringstream topicStream;
    topicStream << "blind/" << updateType;

    std::string devicePayload = payloadStream.str();
    return mqttMessage(topicStream.str().c_str(), devicePayload);
}