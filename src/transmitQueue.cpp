#include <Arduino.h>
#include "transmitQueue.h"
#include "flasher.h"


TransmitQueue::TransmitQueue()
{
}

void TransmitQueue::enqueue(IMessage* message)
{
    messageQueue.push(message);
}

void TransmitQueue::sendNextMessage()
{
    auto now = millis();
    
    if( (now + delayBetweenMessagesMS) < dontSendBeforeMillis)
    {
        //If millis() has wrapped then millis() << dontSendBeforeMillis. Deal with this by resetting broacast time to 100ms from now
        dontSendBeforeMillis = now + delayBetweenMessagesMS;
    }

    if(messageQueue.empty() || now < dontSendBeforeMillis)
    {
        return;
    }

    //TODO when Zemismart is added, only set delay if message is complete
    dontSendBeforeMillis = millis() + delayBetweenMessagesMS;

    auto message = messageQueue.front();
    message->send();
    if(message->isComplete())
    {
        messageQueue.pop();
        delete message;
    }
    flasher::blinkOn(50);
}


