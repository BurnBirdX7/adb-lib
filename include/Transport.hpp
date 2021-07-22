#ifndef ADB_TEST_TRANSPORT_HPP
#define ADB_TEST_TRANSPORT_HPP

#include <functional>
#include "APacket.hpp"


class Transport {
public:
    using SendListener = std::function<void(const APacket&, int errorCode)>;
    using ReceiveListener = std::function<void(const APacket&, int errorCode)>;

public:
    virtual void write(const APacket& packet) = 0;
    virtual void receive() = 0;

    void setSendListener(SendListener);
    void resetSendListener();
    void setReceiveListener(ReceiveListener);
    void resetReceiveListener();

    [[nodiscard]] uint32_t getVersion() const;

protected:
    void notifySendListener(const APacket&, int errorCode);
    void notifyReceiveListener(const APacket&, int errorCode);

    SendListener mSendListener;
    ReceiveListener mReceiveListener;

    uint32_t mVersion = A_VERSION_MIN;
};


#endif //ADB_TEST_TRANSPORT_HPP
