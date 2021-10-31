#include "limits.h"
#include "Rf433Transmitter.h"
#include <Arduino.h>

Rf433Transmitter::Rf433Transmitter(uint8_t tx_gpio)
:tx_gpio(tx_gpio)
{
    pinMode(tx_gpio, OUTPUT);
    TurnOff();
}

void Rf433Transmitter::StartTransmissionTimer()
{
    const unsigned long safeConstantTransmisisonTime = 500UL;
    transmissionPauseTime = millis() + safeConstantTransmisisonTime;

    if(transmissionPauseTime < millis()) //transmissionPauseTime overflowed 
    {
        transmissionPauseTime = ULONG_MAX;
    }
}

void Rf433Transmitter::SendPulse(const RfPulse& pulse)
{
    digitalWrite(tx_gpio, pulse.getLevel());
    delayMicroseconds(pulse.getDurationInMicroSeconds());
}

bool Rf433Transmitter::PauseTransmission()
{
    return millis() > transmissionPauseTime;
}

void Rf433Transmitter::TurnOff()
{
    digitalWrite(tx_gpio, LOW);
}
