#include "Transport.hpp"

#include <utility>

void Transport::setSendListener(Transport::SendListener listener)
{
    mSendListener = std::move(listener);
}

void Transport::setReceiveListener(Transport::ReceiveListener listener)
{
    mReceiveListener = std::move(listener);
}

void Transport::resetSendListener()
{
    mSendListener = {};
}

void Transport::resetReceiveListener()
{
    mReceiveListener = {};
}

void Transport::notifySendListener(const APacket& packet)
{
    if(mSendListener)
        mSendListener(packet);
}

void Transport::notifyReceiveListener(const APacket& packet)
{
    if(mReceiveListener)
        mReceiveListener(packet);
}

uint32_t Transport::getVersion() const
{
    return mVersion;
}
