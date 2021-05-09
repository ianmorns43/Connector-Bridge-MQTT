#include <Arduino.h>
#include "deviceType.h"


//The code returned for different device types in Device List
std::string DeviceType::RFMotor = "10000000";
std::string DeviceType::WiFiCurtain = "22000000";
std::string DeviceType::WiFiBridge = "02000001";
std::string DeviceType::WiFiTubularMotor = "22000002";
std::string DeviceType::WiFiReceiver = "22000005";

std::string DeviceType::ShadeTypeName(unsigned int type)
{
    //Serial.printf("Type value: %d\r\n", type);
    if(type < 1 || type > shadeTypes.size())
    {
        return "Unknown";
    }
    
    return shadeTypes[type-1];
}

std::array<std::string, 14> DeviceType::shadeTypes = {
    "Roller Blinds",
    "Venetian Blinds",
    "Roman Blinds",
    "Honeycomb Blinds",
    "Shangri-La Blinds",
    "Roller Shutter",
    "Roller Gate",
    "Awning",
    "TDBU",
    "Day&night Blinds",
    "Dimming Blinds",
    "Curtain",
    "Curtain(Open Left)",
    "Curtain(Open Right)"
};
