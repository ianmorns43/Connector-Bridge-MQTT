#ifndef FLASHER_H
#define FLASHER_H

#include <Arduino.h>

class flasher
{
    public:
        static void start(uint8_t ledPin);
        static void loop();
        
        static void switchOn();
        static void switchOff();
        static void offForDelay(unsigned long ms);
        static void blinkOn(unsigned long ms);


    private:
        enum Action{none, on, off};
        static uint8_t _ledPin;
        static unsigned long _nextActionTime;
        static Action _nextAction;
        static void switchLed(Action action);

};

#endif