#ifndef ADB_LIB_APACKET_HPP
#define ADB_LIB_APACKET_HPP

#include <memory>
#include <optional>

#include "adb.hpp"
#include "APayload.hpp"

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
                .dataLength = 0,
                .dataCheck = 0,
                .magic = command ^ ALL_ONES_UL};
    }

    [[nodiscard]] inline std::string_view viewCommand() const {
        auto* ptr = reinterpret_cast<const char*>(&command);
        return {ptr, 4};
    }

};

class APacket {
public:
    APacket() = default;
    explicit APacket(const AMessage&);
    APacket(const AMessage&, const APayload&);  // copy payload
    APacket(const AMessage&, APayload&&);       // move payload
    APacket(APacket&&) = default;
    APacket(const APacket&) = default;
    ~APacket() = default;

    void setMessage(const AMessage&);
    void movePayloadIn(APayload&&);
    void copyPayloadIn(const APayload&);

    AMessage& getMessage();
    [[nodiscard]] const AMessage& getMessage() const;
    APayload& getPayload();
    [[nodiscard]] const APayload& getPayload() const;
    [[nodiscard]] bool hasPayload() const;

    APayload movePayloadOut();
    void computeChecksum();
    void resetChecksum();

    void updateMessageDataLength();

private:
    AMessage mMessage = {};
    std::optional<APayload> mPayload;

};

#endif //ADB_LIB_APACKET_HPP
