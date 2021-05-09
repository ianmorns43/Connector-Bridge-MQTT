#include "connectorMqttClient.h"
#include <sstream>
#include "ArduinoJson.h"
#include "mqttParams.h"

WiFiClient ConnectorMqttClient::espClient;
PubSubClient ConnectorMqttClient::mqttClient(espClient);
typedef StaticJsonDocument<2048> JsonDocumentRoot;

void ConnectorMqttClient::publishStatus(const char* topic, const char* payload)
{
    std::ostringstream stream;
    stream << MQTT_TOPIC << "/status/" << topic;
    mqttClient.publish(stream.str().c_str(), payload);
}

void ConnectorMqttClient::mqttConnect()
{
    std::ostringstream lwtTopic;
    lwtTopic << MQTT_TOPIC << "/lwt";
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

            mqttClient.publish(lwtTopic.str().c_str(),"online", 0);
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

    if(command_ptr == nullptr || mac == nullptr)
    {
        //TOOD publish and error message
        return;
    }

    auto command = std::string(command_ptr);
    if(command == "updateDeviceList")
    {
        //TODO Send update device list message
    }
    else if(command == "moveShade")
    {
        auto position_ptr = doc["position"];
        if(position_ptr == nullptr || mac == nullptr)
        {
            //TOOD publish and error message
            return;
        }
        auto position = (int) position_ptr;
        //TODO Send change position message
    }
    else if(command == "open")
    {
        //TODO send open massage
    }
    else if(command == "close")
    {
        //TODO send close message
    }
    else if(command == "updateDevice")
    {
        //Todo send read device message
    }
}

void ConnectorMqttClient::setup()
{           
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
