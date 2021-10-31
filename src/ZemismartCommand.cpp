#include <Arduino.h>
#include <list>
#include "ZemismartCommand.h"
#include "Rf433Transmitter.h"



Rf433Transmitter ZemismartCommand::transmitter(D2);

void ZemismartCommand::send()
{

}

bool ZemismartCommand::isComplete() const
{

}

ZemismartCommand::ZemismartCommand(Command command, const char* remoteId, int channel)
{
    unsigned char commandCode = getCommandCode(command);
    std::list<RfFrame*> frames;
    frames.push_back(createSpacerFrame());
    frames.push_back(createCommandFrame(commandCode, remoteId, channel));

    if(command != Command::Stop)
    {
        frames.push_back(createSpacerFrame());
        frames.push_back(createCommandFrame((0x24, remoteId, channel));
    }
}

RfFrame* ZemismartCommand::createCommandFrame(unsigned char commandCode, const char* remoteId, int channel)
{
    auto zero = createBit(276, 600);
    std::list<RfPulse> empty;

    std::list<unsigned char> data;
    data.push_back(0);

    return new RfFrame(1, empty, empty, empty, zero, data);
}

RfFrame* ZemismartCommand::createSpacerFrame()
{
    auto zero = createBit(276, 600);
    std::list<RfPulse> empty;

    std::list<unsigned char> data;
    data.push_back(0);

    return new RfFrame(1, empty, empty, empty, zero, data);
}

std::list<RfPulse> ZemismartCommand::createBit(unsigned long usHigh, unsigned long usLow)
{
    std::list<RfPulse> bit;
    RfPulse high(usHigh, RfLevel::High);
    RfPulse low(usLow, RfLevel::Low);

    bit.push_back(high);
    bit.push_back(low);

    return bit;
}

int ZemismartCommand::getCommandCode(ZemismartCommand::Command command)
{
    switch(command)
    {
        case Command::Open:
            return 0x0B;
        case Command::Close:
            return 0x43;
        case Command::Stop:
            return 0x23;
    };
}
/*
{
"remoteid": "A33E7A5E",
"signalid": "up",
"frames": [
{
    "repeats": 1,
    "leadin": [],
     "one":  [],
     "zero": [{"time_us": 276, "level": "high"}, {"time_us": 600, "level": "low"}],
     "data": [0],
     "leadout": []
},
{
    "repeats": 6,
    "leadin": [{"time_us": 5000, "level": "high"},{"time_us": 614, "level": "low" }],
    "one": [{"time_us": 600, "level": "high"},{"time_us": 276, "level": "low"}],
    "zero": [{"time_us": 276, "level": "high"},{"time_us": 600, "level": "low"}],
    "data": [A3 3E 7A 5E 02 00 0B 22],
    "leadout": [{"time_us": 600, "level": "high"},{"time_us": 5000, "level": "low" }]  
},
{
    "repeats": 1,
    "leadin": [],
     "one":  [],
     "zero": [{"time_us": 276, "level": "high"}, {"time_us": 600, "level": "low"}],
     "data": [0],
     "leadout": []
},
{
    "repeats": 6,
    "leadin": [{"time_us": 5000, "level": "high"},{"time_us": 614, "level": "low" }],
    "one": [{"time_us": 600, "level": "high"},{"time_us": 276, "level": "low"}],
    "zero": [{"time_us": 276, "level": "high"},{"time_us": 600, "level": "low"}],
    "data": [A3 3E 7A 5E 02 00 24 3C],
    "leadout": [{"time_us": 600, "level": "high"},{"time_us": 5000, "level": "low" }]  
}
]
}*/