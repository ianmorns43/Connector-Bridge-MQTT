#include <Arduino.h>
#include "udpMessage.h"
#include "connectorUdp.h"



udpMessage::udpMessage(const char* body)
    :body(body), sendAttemptsRemaining(3)
{}

void udpMessage::send()
{
    bool success = send(body);
    sendAttemptsRemaining = success ? 0 : sendAttemptsRemaining-1;
}

bool udpMessage::isComplete() const
{
    return sendAttemptsRemaining <= 0;
}

multicastUdpMessage::multicastUdpMessage(const char* body)
    :udpMessage(body)
{}

bool multicastUdpMessage::send(const std::string& body)
{
    return ConnectorUdp::sendMulticast(body);
}

unicastUdpMessage::unicastUdpMessage(const char* body)
    :udpMessage(body)
{}

bool unicastUdpMessage::send(const std::string& body)
{
    return ConnectorUdp::sendUnicast(body);
}

