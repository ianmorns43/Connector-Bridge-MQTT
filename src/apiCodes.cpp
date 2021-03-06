#include <Arduino.h>
#include "apiCodes.h"
#include <string>
#include <sstream>

//The code returned for different device types in Device List
std::string ApiCodes::RFMotor = "10000000";
std::string ApiCodes::WiFiCurtain = "22000000";
std::string ApiCodes::WiFiBridge = "02000001";
std::string ApiCodes::WiFiTubularMotor = "22000002";
std::string ApiCodes::WiFiReceiver = "22000005";

std::string ApiCodes::ShadeTypeName(unsigned int type)
{
    //Serial.printf("Type value: %d\r\n", type);
    //Dont know why but a Pleated Blind is being reported as type 42 (either that or I set the blind up incorrectly)
    if(type == 42)
    {
        return "Pleated Blind";
    }

    if(type < 1 || type > shadeTypes.size())
    {
        std::ostringstream payloadStream;
        payloadStream << "Unknown (" << type << ")";
        return payloadStream.str();
    }
    
    return shadeTypes[type-1];
}

std::string ApiCodes::OperationName(unsigned int operation)
{
    if(operation >= operationNames.size())
    {
        return "Unknown";
    }
    
    return operationNames[operation];
}

std::array<std::string, 14> ApiCodes::shadeTypes = {
    "Roller Blind",
    "Venetian Blind",
    "Roman Blind",
    "Honeycomb Blind",
    "Shangri-La Blind",
    "Roller Shutter",
    "Roller Gate",
    "Awning",
    "TDBU",
    "Day and Night Blind",
    "Dimming Blind",
    "Curtain",
    "Curtain(Open Left)",
    "Curtain(Open Right)"
};

std::array<std::string, 6> ApiCodes::operationNames = {
    "close",
    "open",
    "stop",
    "unknown(3)",
    "unknown(4)",
    "statusQuery"
};
