#ifndef RF_PULSE_H
#define RF_PULSE_H

#include <Arduino.h>

enum RfLevel{High=HIGH, Low=LOW, Unknown=-1};

class RfPulse
{    
    private:
    ushort microseconds;
    RfLevel level;

    public:
    RfPulse(ushort microseconds, RfLevel level);

    ushort getDurationInMicroSeconds() const;
    RfLevel getLevel() const;
};

#endif