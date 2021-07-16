#include <cstddef>
#include <cassert>

#include "APacket.hpp"

unsigned char* SimplePayload::getData() const
{
    return data;
}

size_t SimplePayload::getLength() const
{
    return length;
}

void APacket::setNewPayload(AbstractPayload* newPayload, bool computeChecksum)
{
    payload = newPayload;
    message.dataLength = newPayload->getLength();
    if (computeChecksum)
        message.dataCheck = APacket::computeChecksum(newPayload);
    else
        message.dataCheck = 0;

}

uint32_t APacket::computeChecksum(AbstractPayload* payload)
{
    assert(payload != nullptr);

    uint32_t checksum = 0;
    const auto length = payload->getLength();
    const auto* data = payload->getData();
    for(size_t i = 0; i < length; ++i)
        checksum += data[i];
    return checksum;
}

void APacket::setNewMessage(const AMessage& newMessage, bool computeChecksum)
{
    message.command = newMessage.command;
    message.arg0 = newMessage.arg0;
    message.arg1 = newMessage.arg1;
    message.magic = newMessage.command ^ ALL_ONES_UL; // we don't know if newMessage's `magic` field is computed so we compute it ourself

    if (payload == nullptr) {
        message.dataCheck = 0;
        message.dataLength = 0;
        return;
    }

    setNewPayload(payload, computeChecksum); // set our own payload again
}
