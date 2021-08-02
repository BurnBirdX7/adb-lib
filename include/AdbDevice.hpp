#ifndef ADB_LIB_ADBDEVICE_HPP
#define ADB_LIB_ADBDEVICE_HPP

#include "AdbBase.hpp"


// Streams:
class AStream;
class AIStream;
class AOStream;

class AdbDevice
        : protected AdbBase
{
public:
    using UniqueTransport = Transport::UniquePointer;

public:
    explicit AdbDevice(UniqueTransport&& transport);

    void setFeatures(FeatureSet features);

    void connect();

    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] bool isAwaitingConnection() const;

private:
    void processConnect(const APacket&);


    void receiveListener(const APacket& packet);

    FeatureSet mFeatureSet;
};

#endif //ADB_LIB_ADBDEVICE_HPP
