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

void Transport::notifySendListener(const APacket* packet, ErrorCode errorCode)
{
    if(mSendListener)
        mSendListener(packet, errorCode);
}

void Transport::notifyReceiveListener(const APacket* packet, ErrorCode errorCode)
{
    if(mReceiveListener)
        mReceiveListener(packet, errorCode);
}


void Transport::setVersion(uint32_t version) {
    mVersion = version;
}

uint32_t Transport::getVersion() const
{
    return mVersion;
}

void Transport::setMaxPayloadSize(size_t maxPayloadSize) {
    mMaxPayloadSize = maxPayloadSize;
}

size_t Transport::getMaxPayloadSize() const {
    return mMaxPayloadSize;
}
