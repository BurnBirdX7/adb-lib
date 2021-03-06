#ifndef ADB_LIB_TRANSPORT_HPP
#define ADB_LIB_TRANSPORT_HPP

#include <functional>
#include "APacket.hpp"


class Transport {
public:
    enum ErrorCode {
        OK = 0,
        CANCELLED,
        TRANSPORT_ERROR,
        TRANSPORT_DISCONNECTED,
        UNDERLYING_ERROR
    };

    // TODO: Change listener model, received packets should be moved out of the Transport
    using Listener = std::function<void(const APacket*, ErrorCode errorCode)>;
    using UniquePointer = std::unique_ptr<Transport>;

public:
    virtual void send(APacket&& packet) = 0;
    virtual void receive() = 0;

    void setSendListener(Listener);
    void setReceiveListener(Listener);
    void setMaxPayloadSize(size_t maxPayloadSize);
    void resetSendListener();
    void resetReceiveListener();

    [[nodiscard]] size_t getMaxPayloadSize() const;

protected:
    void notifySendListener(const APacket*, ErrorCode errorCode);
    void notifyReceiveListener(const APacket*, ErrorCode errorCode);

    Listener mSendListener;
    Listener mReceiveListener;

    size_t mMaxPayloadSize = MAX_PAYLOAD_V1;
};


#endif //ADB_LIB_TRANSPORT_HPP
