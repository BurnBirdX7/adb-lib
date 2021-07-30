#include <cstddef>
#include <cassert>

#include "APacket.hpp"


APacket::APacket(const AMessage& msg)
    : mMessage(msg)
{}

APacket::APacket(const AMessage& msg, const APayload& pd)
    : mMessage(msg)
    , mPayload(pd)
{
}

APacket::APacket(const AMessage& msg, APayload&& pd)
    : mMessage(msg)
    , mPayload(std::move(pd))
{
}

void APacket::setMessage(const AMessage& newMessage)
{
    mMessage = newMessage;
}

void APacket::movePayloadIn(APayload&& other)
{
    mPayload = std::move(other); // calls APayload::operator=(APayload&&)
}

void APacket::copyPayloadIn(const APayload& other)
{
    mPayload = other; // calls APayload::operator=(const APayload&)
}

AMessage& APacket::getMessage()
{
    return mMessage;
}

const AMessage& APacket::getMessage() const
{
    return mMessage;
}

APayload& APacket::getPayload()
{
    assert(mPayload.has_value() && "Tried to get payload from APacket with no payload");
    return *mPayload;
}


const APayload& APacket::getPayload() const
{
    assert(mPayload.has_value() && "Tried to get payload from APacket with no payload");
    return *mPayload;
}

bool APacket::hasPayload() const
{
    return mPayload.has_value();
}

APayload APacket::movePayloadOut()
{
    assert(mPayload.has_value() && "Tried to move payload from APacket with no payload");
    APayload pd = std::move(*mPayload);
    mPayload.reset();
    return pd;
}

void APacket::computeChecksum()
{
    if(!mPayload) {
        mMessage.dataCheck = 0;
        return;
    }

    mMessage.dataCheck = 0;
    const auto& payload = *mPayload;
    for(size_t i = 0; i < payload.getSize(); ++i)
        mMessage.dataCheck += payload[i];
}

void APacket::resetChecksum()
{
    mMessage.dataCheck = 0;
}

void APacket::updateMessageDataLength() {
    if (mPayload.has_value())
        mMessage.dataLength = mPayload->getSize();
}
