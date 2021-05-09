#ifndef DEVICE_TYPE_H
#define DEVICE_TYPE_H

#include <string>
#include <array>

class DeviceType
{
    public:

    static std::string RFMotor;
    static std::string WiFiCurtain;
    static std::string WiFiBridge;
    static std::string WiFiTubularMotor;
    static std::string WiFiReceiver;

    static std::string ShadeTypeName(unsigned int type);

    private:
    DeviceType(){}

    static std::array<std::string, 14> shadeTypes;
};

#endif