#ifndef RF_FRAME_H
#define RF_FRAME_H

#include <list>
#include "RfPulse.h"
#include "IRf433Transmitter.h"

class RfFrame
{
    public:

    RfFrame(int repeats, const std::list<RfPulse>& leadin, const std::list<RfPulse>& leadout, const std::list<RfPulse>& one, const std::list<RfPulse>& zero, const std::list<unsigned char>& data);
    void Send(IRf433Transmitter& tx);
    bool Complete() const;

    private:
    
    void Send(IRf433Transmitter& tx, const std::list<RfPulse>& pulses) const;
    
    int repeats;
    int repeatsSent;
    std::list<RfPulse> leadin;
    std::list<RfPulse> leadout;
    std::list<RfPulse> one;
    std::list<RfPulse> zero;
    std::list<unsigned char> data;
};


#endif