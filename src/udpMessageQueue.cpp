#include <Arduino.h>
#include "udpMessageQueue.h"
#include "flasher.h"


udpMessageQueue::udpMessageQueue()
{
}

void udpMessageQueue::enqueue(IMessage* message)
{
    messageQueue.push(message);
}

void udpMessageQueue::sendNextMessage()
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


