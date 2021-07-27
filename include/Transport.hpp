#ifndef ADB_TEST_TRANSPORT_HPP
#define ADB_TEST_TRANSPORT_HPP

#include <functional>
#include "APacket.hpp"


class Transport {
public:
    enum ErrorCode {
        OK = 0,
        CANCELLED,
        TRANSPORT_ERROR,
        UNDERLYING_ERROR
    };

    using Listener = std::function<void(const APacket*, ErrorCode errorCode)>;

public:
    virtual void send(APacket&& packet) = 0;
    virtual void receive() = 0;

    void setSendListener(Listener);
    void setReceiveListener(Listener);

    virtual void setVersion(uint32_t version);
    virtual void setMaxPayloadSize(size_t maxPayloadSize);
    void resetSendListener();
    void resetReceiveListener();

    [[nodiscard]] uint32_t getVersion() const;
    [[nodiscard]] size_t getMaxPayloadSize() const;

protected:
    void notifySendListener(const APacket*, ErrorCode errorCode);
    void notifyReceiveListener(const APacket*, ErrorCode errorCode);

    Listener mSendListener;
    Listener mReceiveListener;

    uint32_t mVersion = A_VERSION_MIN;
    size_t mMaxPayloadSize = MAX_PAYLOAD_V1;
};


#endif //ADB_TEST_TRANSPORT_HPP
