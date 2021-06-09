#include <Arduino.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

class OTA
{
    public:
    static void setup(const char* hostname)
    {
        ArduinoOTA.onStart([]() 
        {
            String type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
            Serial.println("Start updating " + type);
        });
        ArduinoOTA.onEnd([]()
        {
            Serial.println("\nEnd");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
        {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) 
        {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
        });

        ArduinoOTA.setHostname(hostname);
        ArduinoOTA.begin();
        Serial.printf("OTA Server ready on %s\r\n", hostname);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }

    static void loop()
    {
        ArduinoOTA.handle();
    }
};