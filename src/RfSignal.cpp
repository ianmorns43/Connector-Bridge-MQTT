#include <sstream>
#include "RfSignal.h"



RfSignal::RfSignal(const std::string& remoteid, const std::string& signalid)
:remoteId(remoteid), signalId(signalid), frames()
{
    currentFrame = frames.begin();
}

RfSignal::~RfSignal()
{
    for(auto it = frames.begin(); it != frames.end(); ++it)
    {
        delete *it;
    }
    frames.clear();
}

void RfSignal::SendNextFrame(IRf433Transmitter& tx)
{
    (*currentFrame)->Send(tx);
    if((*currentFrame)->Complete())
    {
        ++currentFrame;
    }
}

bool RfSignal::Complete() const
{
    return currentFrame == frames.end();
}

const char* RfSignal::getRemoteId() const
{
    return remoteId.c_str();
}

const char* RfSignal::getSignalId() const
{
    return signalId.c_str();
}

