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
    ~AdbDevice() override;

    void setFeatures(FeatureSet features);

    void connect();

    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] bool isAwaitingConnection() const;

private:
    void processConnect(const APacket&);
    void processOpen(const APacket&);
    void processReady(const APacket&);
    void processClose(const APacket&);
    void processWrite(const APacket&);
    void processAuth(const APacket&);
    void processTls(const APacket&);

    void packetListener(const APacket& packet);
    void errorListener(int errorCode, const APacket* packet, bool incomingPacket);

    FeatureSet mFeatureSet;

    // Details:
    std::string mSerial  = {};
    std::string mProduct = {};
    std::string mModel   = {};
    std::string mDevice  = {};
};

#endif //ADB_LIB_ADBDEVICE_HPP
