#ifndef IRF433_TRANSMITTER_H
#define IRF433_TRANSMITTER_H

#include "RfPulse.h"

class IRf433Transmitter
{
    public:
    virtual void SendPulse(const RfPulse& pulse) = 0;
    virtual bool PauseTransmission() = 0;
    virtual void TurnOff() = 0;
};

#endif