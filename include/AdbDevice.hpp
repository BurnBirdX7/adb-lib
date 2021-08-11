#ifndef ADB_LIB_ADBDEVICE_HPP
#define ADB_LIB_ADBDEVICE_HPP

#include <map>
#include <vector>
#include <condition_variable>

#include "AdbBase.hpp"
#include "AdbStreams.hpp"


class AdbDevice
        : protected AdbBase
        , public std::enable_shared_from_this<AdbDevice>
{
public:
    using SharedPointer = std::shared_ptr<AdbDevice>;
    using UniqueTransport = Transport::UniquePointer;

    struct Streams {
        AdbIStream istream;
        AdbOStream ostream;
    };


public:
    static SharedPointer make(UniqueTransport&& transport);
    ~AdbDevice() override;

    // Features setup
    void setFeatures(FeatureSet features);

    // Keys' setup
    void setPrivateKeyPaths(std::vector<std::string> paths);
    void addPrivateKeyPath(const std::string_view& path);
    void setPublicKeyPath(const std::string_view& path);

    const std::string& getSerial() const;
    const std::string& getProduct() const;
    const std::string& getModel() const;
    const std::string& getDevice() const;

    void connect();
    std::optional<Streams> open(const std::string_view& destination);

    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] bool isAwaitingConnection() const;

private:
    explicit AdbDevice(UniqueTransport&& transport);

public: // Stream's actions
    void closeStream(uint32_t localId);
    void send(uint32_t localId, uint32_t remoteId, APayload&& payload);

private: // Packet processing
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
    std::optional<APayload> signWithPrivateKey(const APayload& hash);
    void sendPublicKey();

private:
    FeatureSet mFeatureSet;

    // Details:
    std::string mSerial  = {};
    std::string mProduct = {};
    std::string mModel   = {};
    std::string mDevice  = {};

    // Auxiliary:
    std::condition_variable mConnected;

    // StreamBases:
    using StreamBase = std::weak_ptr<AdbStreamBase>;

    struct AwaitingStream {
        std::condition_variable cv = {};
        uint32_t remoteId = 0;
        bool rejected = false;
    };

    uint32_t mLastLocalId;
    std::mutex mStreamsMutex;
    std::map<uint32_t /*localId*/, StreamBase> mActiveStreams;
    std::map<uint32_t /*localId*/, AwaitingStream> mAwaitingStreams;

    std::vector<std::string> mPrivateKeyPaths;
    size_t mLastTriedKey = 0;
    std::string mPublicKeyPath;
    bool mPublicIsAlreadyTried = false;
};

#endif //ADB_LIB_ADBDEVICE_HPP
