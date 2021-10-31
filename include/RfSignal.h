#ifndef RF_SIGNAL_H
#define RF_SIGNAL_H

#include <queue>
#include "RfFrame.h"
#include "IRf433Transmitter.h"

class RfSignal
{
    public:

    ~RfSignal();

    void SendNextFrame(IRf433Transmitter& tx);
    bool Complete() const;

    const char* getRemoteId() const;
    const char* getSignalId() const;
    
    private:
    RfSignal(const std::string& remoteid, const std::string& signalid);

    std::string remoteId;
    std::string signalId;
    std::list<RfFrame*> frames;
    std::list<RfFrame*>::iterator currentFrame;
};


#endif