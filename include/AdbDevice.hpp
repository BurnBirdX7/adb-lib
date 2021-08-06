#ifndef ADB_LIB_ADBDEVICE_HPP
#define ADB_LIB_ADBDEVICE_HPP

#include <map>
#include <condition_variable>

#include "AdbBase.hpp"


// Streams:
class AStream;
class AIStream;
class AOStream;

class AdbDevice
        : protected AdbBase
        , public std::enable_shared_from_this<AdbDevice>
{
public:
    using SharedPointer = std::shared_ptr<AdbDevice>;
    using UniqueTransport = Transport::UniquePointer;

    struct StreamsRef {
        AIStream& istream;
        AOStream& ostream;
    };


public:
    static SharedPointer make(UniqueTransport&& transport);
    ~AdbDevice() override;

    void setFeatures(FeatureSet features);

    void connect();
    std::optional<StreamsRef> open(const std::string_view& destination);

    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] bool isAwaitingConnection() const;

private:
    explicit AdbDevice(UniqueTransport&& transport);

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

private:
    FeatureSet mFeatureSet;

    // Details:
    std::string mSerial  = {};
    std::string mProduct = {};
    std::string mModel   = {};
    std::string mDevice  = {};

    // Auxiliary:
    std::condition_variable mConnected;

    // Streams:
    struct Streams {
        std::unique_ptr<AIStream> istream;
        std::unique_ptr<AOStream> ostream;
    };

    struct StreamsAwaiting {
        std::condition_variable cv = {};
        uint32_t remoteId = 0;
        bool rejected = false;
    };

    std::mutex mStreamsMutex;
    std::map<uint32_t /*localId*/, Streams> mStreams;

    uint32_t mLastLocalId;
    std::map<uint32_t /*localId*/, StreamsAwaiting> mAwaitingStreams;

private:
    friend AIStream;
    friend AOStream;

};

#endif //ADB_LIB_ADBDEVICE_HPP
