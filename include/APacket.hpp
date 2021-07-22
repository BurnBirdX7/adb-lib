#ifndef ADB_TEST_APACKET_HPP
#define ADB_TEST_APACKET_HPP

#include <memory>
#include "adb.hpp"

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
    virtual ~AbstractPayload() = default;

    [[nodiscard]] virtual unsigned char* getData() const = 0;
    [[nodiscard]] virtual size_t getLength() const = 0;
    virtual void resize(size_t newSize) = 0;
};

struct SimplePayload
        : public AbstractPayload
{
    explicit SimplePayload(size_t length);
    SimplePayload(unsigned char* data, size_t length);
    SimplePayload(SimplePayload&&) = default;
    SimplePayload(const SimplePayload& other);
    ~SimplePayload() override;

    [[nodiscard]] unsigned char* getData() const override;
    [[nodiscard]] size_t getLength() const override;
    void resize(size_t newSize) override;

    unsigned char* data = nullptr;
    size_t length       = 0;
};

struct APacket {
    AMessage message                = {};
    AbstractPayload* payload        = nullptr;
    bool deletePayloadOnDestruction = true; // if this is true, deletes payload on destruction

    ~APacket();

    void setNewMessage(const AMessage& newMessage, bool computeChecksum);
    void setNewPayload(AbstractPayload* newPayload, bool computeChecksum, bool deletePayloadOnDestruction = true);
    static uint32_t computeChecksum(AbstractPayload* payload);

};

#endif //ADB_TEST_APACKET_HPP
