#ifndef TIME_STAMP_GENERATOR_H
#define TIME_STAMP_GENERATOR_H

#include <ezTime.h>
#include <string>
#include <sstream>
#include <iomanip>

class TimeStampGenerator
{
    public:
    TimeStampGenerator() : myTZ()
    {}

    void start()
    {
        waitForSync();
        myTZ.setLocation(F("gb"));
    }

    std::string Generate()
    {
        std::ostringstream stream;
        stream << std::setfill('0') << std::setw(4) << myTZ.year() << std::setw(2) << (int)myTZ.month() << std::setw(2) << (int)myTZ.day() << std::setw(2) << (int) myTZ.hour() << std::setw(2) << (int)myTZ.minute() << std::setw(2) << (int)myTZ.second() << std::setw(2) << myTZ.ms();

        return std::string(stream.str());
    }

    private:
        Timezone myTZ;
};


#endif