#ifndef RF433_TRANSMITTER_H
#define RF433_TRANSMITTER_H

#include "IRf433Transmitter.h"
#include "RfPulse.h"


class Rf433Transmitter : public IRf433Transmitter
{
    public:
    Rf433Transmitter(uint8_t tx_gpio);
    void StartTransmissionTimer();
    void SendPulse(const RfPulse& pulse);
    bool PauseTransmission();
    void TurnOff();

    private:
    uint8_t tx_gpio;
    unsigned long transmissionPauseTime;
};

#endif