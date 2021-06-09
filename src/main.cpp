#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <string>
#include <sstream>
#include "OTA.h"
#include "udpMessageQueue.h"
#include "accessTokenGenerator.h"
#include "connectorUdp.h"
#include "connectorMqttClient.h"

#define READ_WRITE_PIN gpio_num_t::GPIO_NUM_2
enum READ_WRITE{Write=HIGH, Read=LOW};

udpMessageQueue udpMessages;
ConnectorUdp udp(udpMessages);

void setup()
{
  Serial.begin(9600);
  Serial.println();

  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.println("Connecting to WIFI...");
  
  wifi_station_set_hostname(MQTT_TOPIC);
  WiFiManager wifimanager;
  wifimanager.autoConnect(MQTT_TOPIC);

  Serial.println("Connecting to mqtt broker");
  ConnectorMqttClient::setup(udpMessages);

  Serial.println("Opening UDP socket...");
  udpMessages.beginListening();
  udp.start();

  Serial.println("Initialising OTA update service...");
  std::ostringstream stream;
  stream <<"esp8266-" << MQTT_TOPIC;
  OTA::setup(stream.str().c_str());
  Serial.println("Ready...");
}

void loop() 
{
  OTA::loop();
  auto mqttMessage = udp.loop();
  if(mqttMessage.empty())
  {
    udpMessages.sendNextMessage();
  }
  else
  {
    mqttMessage.publish(ConnectorMqttClient::mqttClient);
  }
  ConnectorMqttClient::loop();
}