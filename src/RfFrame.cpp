#include "RfFrame.h"


RfFrame::RfFrame(int repeats, const std::list<RfPulse>& leadin, const std::list<RfPulse>& leadout, const std::list<RfPulse>& one, const std::list<RfPulse>& zero, const std::list<unsigned char>& data)
: repeats(repeats),
repeatsSent(0),
leadin(leadin),
leadout(leadout),
one(one),
zero(zero),
data(data)
{
}
    
void RfFrame::Send(IRf433Transmitter& tx, const std::list<RfPulse>& pulses) const
{
    for(auto it = pulses.begin(); it != pulses.end(); ++it)
    {
        tx.SendPulse(*it);
    }
}

bool RfFrame::Complete() const
{
    return repeatsSent >= repeats;
}

void RfFrame::Send(IRf433Transmitter& tx)
{
    while(!tx.PauseTransmission() && repeatsSent++ < repeats)
    {
        Send(tx, leadin);
        for(auto da = data.begin(); da != data.end(); ++da)
        {
            for(unsigned char mask = 128; mask > 0; mask >>= 1)
            {
                Send(tx, (mask & *da) > 0 ? one : zero);
            }   
        }
        Send(tx, leadout);
    }
}