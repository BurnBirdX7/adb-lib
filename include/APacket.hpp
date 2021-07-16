#ifndef ADB_TEST_APACKET_HPP
#define ADB_TEST_APACKET_HPP

#include "libadb.hpp"

constexpr uint32_t ALL_ONES_UL = ~uint32_t(0);

struct AMessage {
    using field = uint32_t;

    field command;    // command identifier constant
    field arg0;       // first argument
    field arg1;       // second argument
    field dataLength; // length of payload (0 is allowed)
    field dataCheck;  // checksum of data payload
    field magic;      // command ^ 0xffffffff

    inline static AMessage make(field command, field arg0, field arg1) {
        return {.command = command,
                .arg0 = arg0,
                .arg1 = arg1,
                .magic = command ^ ALL_ONES_UL};
    }
};


class AbstractPayload {
public:
    [[nodiscard]] virtual unsigned char* getData() const = 0;
    [[nodiscard]] virtual size_t getLength() const = 0;
};


struct SimplePayload
        : public AbstractPayload
{
    [[nodiscard]] unsigned char* getData() const override;
    [[nodiscard]] size_t getLength() const override;

    unsigned char* data = nullptr;
    size_t length       = 0;
};

struct APacket {
    AMessage message         = {};
    AbstractPayload* payload = nullptr;

    void setNewMessage(const AMessage& newMessage, bool computeChecksum);
    void setNewPayload(AbstractPayload* newPayload, bool computeChecksum);
    static uint32_t computeChecksum(AbstractPayload* payload);

};

#endif //ADB_TEST_APACKET_HPP
