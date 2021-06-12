#ifndef IMESSAGE_H
#define IMESSAGE_H

class IMessage
{
    public:

    virtual void send() = 0;
    virtual bool isComplete() const = 0;

};

#endif