#ifndef ADB_TEST_TRANSPORT_HPP
#define ADB_TEST_TRANSPORT_HPP

#include <functional>
#include "APacket.hpp"

class Transport {
public:
    using SendListener = std::function<void(const APacket&)>;
    using ReceiveListener = std::function<void(const APacket&)>;

public:
    virtual void write(const APacket& packet) = 0;
    virtual void startReceiving() = 0;

    void setSendListener(SendListener);
    void resetSendListener();
    void setReceiveListener(ReceiveListener);
    void resetReceiveListener();

    [[nodiscard]] uint32_t getVersion() const;

protected:
    void notifySendListener(const APacket&);
    void notifyReceiveListener(const APacket&);

    SendListener mSendListener;
    ReceiveListener mReceiveListener;

    uint32_t mVersion = A_VERSION_MIN;

};

#endif //ADB_TEST_TRANSPORT_HPP
