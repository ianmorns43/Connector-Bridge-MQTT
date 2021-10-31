#ifndef ZEMISMART_COMMAND_H
#define ZEMISMART_COMMAND_H

#include "IMessage.h"
#include "Rf433Transmitter.h"
#include "RfSignal.h"

class ZemismartCommand : public IMessage
{
    public:

    enum Command{Open, Close, Stop};
    ZemismartCommand(Command command, const char* remoteId, int channel);
    ~ZemismartCommand();

    void send();
    bool isComplete() const;

    private:

    static int getCommandCode(ZemismartCommand::Command command);
    static std::list<RfPulse> createBit(unsigned long usHigh, unsigned long usLow);
    static RfFrame* createSpacerFrame();
    static RfFrame* createCommandFrame(unsigned char commandCode, const char* remoteId, int channel);

    static Rf433Transmitter transmitter;

};


#endif