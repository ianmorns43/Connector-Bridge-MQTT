#include <Arduino.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <string>
#include "timeStampGenerator.h"
#include "accessTokenGenerator.h"

#define READ_WRITE_PIN gpio_num_t::GPIO_NUM_2
enum READ_WRITE{Write=HIGH, Read=LOW};

WiFiUDP multicastUpd;
//WiFiUDP localUdp;
unsigned int hubResponsePort = 32101;  // port to listen on
unsigned int sendPort = 32100;  // port to send on
IPAddress multicastIP(238,0,0,18);


TimeStampGenerator timestamp;

void setup()
{
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  //randomSeed(analogRead(0));

  Serial.println();
  Serial.println("Connecting to WIFI...");
  
  wifi_station_set_hostname(MQTT_TOPIC);
  WiFiManager wifimanager;
  wifimanager.autoConnect(MQTT_TOPIC);

  timestamp.start();

  //Listen for broadcast messages
  multicastUpd.beginMulticast(WiFi.localIP(), multicastIP, hubResponsePort);
  Serial.printf("Now listening to %s:%d\n", multicastIP.toString().c_str(), hubResponsePort);

  Serial.println("Asking hub to identify itself");
  const String getDeviceList = "{ \"msgType\":\"GetDeviceList\",\"msgID\":\"20210508134209916\" }";

  multicastUpd.beginPacket(multicastIP, sendPort);
  multicastUpd.write(getDeviceList.c_str(), getDeviceList.length());
  multicastUpd.endPacket();
}

bool cmdSent = false;
const unsigned int incomingPacketBufferSize = 1024;
char incomingPacket[incomingPacketBufferSize];  // buffer for incoming packets
void loop() 
{
  int packetSize = multicastUpd.parsePacket();
  if (packetSize)
  {
    Serial.printf("Received %d bytes from %s:%d, on %s:%d\n", packetSize, multicastUpd.remoteIP().toString().c_str(), multicastUpd.remotePort(), multicastUpd.destinationIP().toString().c_str(), hubResponsePort);
    multicastUpd.read(incomingPacket, incomingPacketBufferSize);
    Serial.println(incomingPacket);

    /*if(!cmdSent)
    {
      const String posTo44 = "{\"msgType\":\"WriteDevice\",\"mac\":\"f008d1edd4ec0002\",\"deviceType\":\"10000000\",\"AccessToken\":\"***********\",\"msgID\":\"20210508144209916\",\"data\":{\"targetPosition\":44}}";
      multicastUpd.beginPacket(multicastIP, sendPort);
      multicastUpd.write(posTo44.c_str(), posTo44.length());
      multicastUpd.endPacket();
      cmdSent = true;
    }*/
  }
}