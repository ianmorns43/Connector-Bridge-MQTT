#include <Arduino.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

#define READ_WRITE_PIN gpio_num_t::GPIO_NUM_2
enum READ_WRITE{Write=HIGH, Read=LOW};

WiFiUDP multicastUpd;
WiFiUDP localUdp;
unsigned int hubResponsePort = 32101;  // port to listen on
unsigned int sendPort = 32100;  // port to send on
IPAddress multicastIP(238,0,0,18);

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

  //Listen for broadcast messages
  multicastUpd.beginMulticast(WiFi.localIP(), multicastIP, hubResponsePort);
  Serial.printf("Now listening to %s:%d\n", multicastIP.toString().c_str(), hubResponsePort);

  localUdp.begin(hubResponsePort);
  Serial.printf("Now listening to %s:%d\n", WiFi.localIP().toString().c_str(), hubResponsePort);
  
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println("Asking hub to identify itself");
  const String getDeviceList = "{ \"msgType\":\"GetDeviceList\",\"msgID\":\"20200321134209916\" }";

  localUdp.beginPacket(multicastIP, sendPort);
  localUdp.write((const unsigned char*)getDeviceList.c_str(), getDeviceList.length());
  localUdp.endPacket();
}


const unsigned int incomingPacketBufferSize = 1024;
char incomingPacket[incomingPacketBufferSize];  // buffer for incoming packets
void loop() 
{
  int packetSize = multicastUpd.parsePacket();
  if (packetSize)
  {
    Serial.printf("Received %d bytes from %s:%d, on %s:%d\n", packetSize, multicastUpd.remoteIP().toString().c_str(), multicastUpd.remotePort(), multicastIP.toString().c_str(), hubResponsePort);
    multicastUpd.read(incomingPacket, incomingPacketBufferSize);
    Serial.println(incomingPacket);
  }

  packetSize = localUdp.parsePacket();
  if (packetSize)
  {
    Serial.printf("Received %d bytes from %s:%d, on %s:%d\n", packetSize, localUdp.remoteIP().toString().c_str(), localUdp.remotePort(), WiFi.localIP().toString().c_str(), hubResponsePort);
    localUdp.read(incomingPacket, incomingPacketBufferSize);
    Serial.println(incomingPacket);
  }

}