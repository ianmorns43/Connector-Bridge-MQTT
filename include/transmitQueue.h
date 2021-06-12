#ifndef TRANSMIT_QUEUE_H
#define TRANSMIT_QUEUE_H

#include <WiFiUdp.h>
#include <queue>
#include <string>
#include "IMessage.h"


class TransmitQueue
{
    public:
    TransmitQueue();
    void enqueue(IMessage* message);
    void sendNextMessage();

    private:

    //The Dooya Connector Bridge documentation recommends leaving at least 100ms between messages sent to the bridge
    const int delayBetweenMessagesMS = 500;
    std::queue<IMessage*> messageQueue;
    unsigned int dontSendBeforeMillis = 0;
};

#endif