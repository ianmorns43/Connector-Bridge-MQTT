#include <Arduino.h>
#include "flasher.h"



uint8_t flasher::_ledPin = 0;
unsigned long flasher::_nextActionTime = 0;
flasher::Action flasher::_nextAction = flasher::Action::none;
flasher::Action flasher::_lastAction = flasher::Action::none;

void flasher::switchLed(flasher::Action action)
{
    _lastAction = action;
    switch(action)
    {
        case Action::on:
            digitalWrite(_ledPin, LOW);
        break;
        case Action::off:
            digitalWrite(_ledPin, HIGH);
        break;
        case Action::none:
            //Do nothing
        break;
    };
}

void flasher::switchOn()
{
    switchLed(Action::on);
}
void flasher::switchOff()
{
    switchLed(Action::off);
}

void flasher::start(uint8_t ledPin)
{
    _nextAction = Action::none;
    _ledPin = ledPin;
    pinMode(_ledPin, OUTPUT);
    switchOn();
}

void flasher::offForDelay(unsigned long ms)
{
    switchOff();
    delay(ms);
    switchOn();
}

void flasher::blinkOn(unsigned long ms)
{
    switchOn();
    _nextAction = Action::off;
    _nextActionTime = millis() + ms;
}

void flasher::flash(unsigned long ms)
{
    if(_nextAction == Action::none)
    {
        _nextAction = _lastAction == Action::off ? Action::on : Action::off;
        _nextActionTime = millis() + ms;
    }
}

void flasher::loop()
{
    if(_nextAction != Action::none && millis() >= _nextActionTime)
    {
        switchLed(_nextAction);
        _nextAction = Action::none;
    }
}

