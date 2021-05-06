#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <string>
#include "accessTokenGenerator.h"
#include "connectorUdp.h"

#define READ_WRITE_PIN gpio_num_t::GPIO_NUM_2
enum READ_WRITE{Write=HIGH, Read=LOW};



ConnectorUdp udp;

void setup()
{
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println();
  Serial.println("Connecting to WIFI...");
  
  wifi_station_set_hostname(MQTT_TOPIC);
  WiFiManager wifimanager;
  wifimanager.autoConnect(MQTT_TOPIC);

  Serial.println("Opening UDP socket...");
  udp.start();

  Serial.println("Asking hub to identify itself...");
  udp.sendMulticastDeviceListRequest();
}

void loop() 
{
  udp.loop();
}