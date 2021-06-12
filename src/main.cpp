#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <string>
#include <sstream>
#include "OTA.h"
#include "transmitQueue.h"
#include "udpMessage.h"
#include "accessTokenGenerator.h"
#include "connectorUdp.h"
#include "connectorMqttClient.h"
#include "flasher.h"

#define READ_WRITE_PIN gpio_num_t::GPIO_NUM_2
enum READ_WRITE{Write=HIGH, Read=LOW};

TransmitQueue messageQueue;
ConnectorUdp udp(messageQueue);

void setup()
{
  Serial.begin(9600);
  Serial.println();

  flasher::start(LED_BUILTIN);
  Serial.println("Connecting to WIFI...");
  
  flasher::offForDelay(100);
  
  wifi_station_set_hostname(MQTT_TOPIC);
  WiFiManager wifimanager;
  wifimanager.autoConnect(MQTT_TOPIC);

  flasher::offForDelay(100);

  Serial.println("Connecting to mqtt broker");
  ConnectorMqttClient::setup(messageQueue);

  flasher::offForDelay(100);

  Serial.println("Opening UDP socket...");
  udpMessage::beginListening();
  udp.start();

  flasher::offForDelay(100);

  Serial.println("Initialising OTA update service...");
  std::ostringstream stream;
  stream <<"esp8266-" << MQTT_TOPIC;
  OTA::setup(stream.str().c_str());
  Serial.println("Ready...");

  flasher::switchOff();
}

void loop() 
{
  flasher::loop();
  OTA::loop();
  auto mqttMessage = udp.loop();
  if(mqttMessage.empty())
  {
    messageQueue.sendNextMessage();
  }
  else
  {
    mqttMessage.publish(ConnectorMqttClient::mqttClient);
  }
  ConnectorMqttClient::loop();
}