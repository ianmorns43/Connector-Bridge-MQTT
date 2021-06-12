#include "connectorMqttClient.h"
#include <sstream>
#include "ArduinoJson.h"
#include "mqttParams.h"
#include "connectorUdp.h"

WiFiClient ConnectorMqttClient::espClient;
PubSubClient ConnectorMqttClient::mqttClient(espClient);
typedef StaticJsonDocument<2048> JsonDocumentRoot;

TransmitQueue* ConnectorMqttClient::messageQueue = nullptr;

void ConnectorMqttClient::mqttConnect()
{
    std::ostringstream lwtTopic;
    lwtTopic << MQTT_TOPIC << "/hub/lwt";
    // Loop until we're reconnected
    while (!mqttClient.connected())
    {
        Serial.print("Attempting MQTT connection...");
    
        if (mqttClient.connect(MQTT_TOPIC, MQTT_BROKER_USERNAME, MQTT_BROKER_PASSWORD, lwtTopic.str().c_str(), 0, true, "offline"))
        {
            Serial.println(" succeded.");  
            Serial.printf("\r\nLWT topic is: '%s'\r\n", lwtTopic.str().c_str());  

            std::ostringstream commandTopic;
            commandTopic << MQTT_TOPIC << "/command";
            
            auto result = mqttClient.subscribe(commandTopic.str().c_str());
            Serial.printf("Subscribing to '%s' %s\r\n", commandTopic.str().c_str(), result? "succeded":"failed");

            mqttClient.publish(lwtTopic.str().c_str(),"online", true);
        } 
        else 
        {
            Serial.print(" failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds.");
            // Wait 5 seconds before retrying
            for(int i = 0; i < 5; i++)
            {  
                digitalWrite(LED_BUILTIN, LOW);
                delay(750);
                digitalWrite(LED_BUILTIN, HIGH);
                delay(250);
            }
        }
    }
}

void ConnectorMqttClient::mqttCallback(std::string topic, byte* message, unsigned int length)
{
    std::string payload((char*)message, length);
    
    Serial.println("Recieved message:");
    Serial.printf("Topic: %s\r\n", topic.c_str());
    Serial.printf("Payload: %s\r\n", payload.c_str());
    
    JsonDocumentRoot doc;
    auto error = deserializeJson(doc, payload);
    if(error)
    {
        //TOOD publish and error message
        return;
    }

    auto command_ptr = (const char*) doc["command"];
    auto mac = (const char*) doc["mac"];

    if(command_ptr == nullptr)
    {
        //TOOD publish and error message
        return;
    }

    auto command = std::string(command_ptr);
    if(command == "updateDeviceList")
    {
        messageQueue->enqueue(ConnectorUdp::createMulticastDeviceListRequest());
    }
    else if(command == "getStatus")
    {
        if(mac == nullptr)
        {
            //TOOD publish and error message
            return;
        }
        messageQueue->enqueue(ConnectorUdp::createDeviceStatusRequest(mac));
    }
    else if(command == "moveShade")
    {
        auto position_ptr = doc["position"];
        auto key_ptr = doc["key"];
        if(position_ptr == nullptr || mac == nullptr || key_ptr == nullptr)
        {
            //TOOD publish and error message
            return;
        }

        messageQueue->enqueue(ConnectorUdp::createSetPositionRequest((const char*)key_ptr, mac, (int) position_ptr));
    }
    else if(command == "openShade")
    {
        auto key_ptr = doc["key"];
        if(mac == nullptr || key_ptr == nullptr)
        {
            //TOOD publish and error message
            return;
        }

        messageQueue->enqueue(ConnectorUdp::createOpenRequest((const char*)key_ptr, mac));
    }
    else if(command == "closeShade")
    {
        auto key_ptr = doc["key"];
        if(mac == nullptr || key_ptr == nullptr)
        {
            //TOOD publish and error message
            return;
        }

        messageQueue->enqueue(ConnectorUdp::createCloseRequest((const char*)key_ptr, mac));
    }
    else if(command == "stopShade")
    {
        auto key_ptr = doc["key"];
        if(mac == nullptr || key_ptr == nullptr)
        {
            //TOOD publish and error message
            return;
        }

        messageQueue->enqueue(ConnectorUdp::createStopRequest((const char*)key_ptr, mac));
    }
    else
    {
        Serial.printf("Unrecognised command %s\r\n", command.c_str());
    }
}

void ConnectorMqttClient::setup(TransmitQueue& messageQueueRef)
{           
    messageQueue = &messageQueueRef;
    mqttClient.setServer(MQTT_BROKER_IP, 1883);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(2048);
}

void ConnectorMqttClient::loop() 
{
    if(!mqttClient.loop() || !mqttClient.connected())
    {
        mqttConnect();
    }
}
