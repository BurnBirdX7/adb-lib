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
    virtual APacket receive() = 0; // will block until APacket received

    void setSendListener(SendListener);
    void resetSendListener();
    void setReceiveListener(ReceiveListener);
    void resetReceiveListener();

protected:
    void notifySendListener(const APacket&);    // atm just calls the callbacks
    void notifyReceiveListener(const APacket&);

    SendListener mSendListener;
    ReceiveListener mReceiveListener;

};

#endif //ADB_TEST_TRANSPORT_HPP
