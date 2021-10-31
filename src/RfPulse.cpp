    #include "RfPulse.h"
    

    RfPulse::RfPulse(ushort microseconds, RfLevel level)
    :microseconds(microseconds), level(level)
    {
    }

    ushort RfPulse::getDurationInMicroSeconds() const
    {
        return microseconds;
    }

    RfLevel RfPulse::getLevel() const
    {
        return level;
    }