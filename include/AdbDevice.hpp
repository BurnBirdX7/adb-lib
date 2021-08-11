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

    enum ConnectionState {
        ANY = -1,

        CONNECTING = 0, // No response from the device yet
        AUTHORIZING,    // Sending signed tokens to the device
        UNAUTHORIZED,   // Authorization tokens are exhausted
        NO_PERMISSION,  // Insufficient permissions to connect the device
        OFFLINE,        // ???

        // Connected:
        BOOTLOADER,
        DEVICE,
        HOST,
        RECOVERY,
        SIDELOAD,
        RESCUE
    };

public:
    static SharedPointer make(UniqueTransport&& transport);
    ~AdbDevice() override;

    // Keys' setup
    void setPrivateKeyPaths(std::vector<std::string> paths);
    void addPrivateKeyPath(const std::string_view& path);
    void setPublicKeyPath(const std::string_view& path);

    const std::string& getSerial() const;
    const std::string& getProduct() const;
    const std::string& getModel() const;
    const std::string& getDevice() const;

    const std::string& getSystemType() const;
    uint32_t getConnectionState() const;
    const FeatureSet& getFeatures() const;

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

    std::optional<APayload> signWithPrivateKey(const APayload& hash);
    void sendPublicKey();

private:
    void setConnectionState(ConnectionState state);
    bool setSystemType(const std::string_view& systemType);

    FeatureSet mFeatureSet;
    ConnectionState mConnectionState;
    std::string mSystemType;

    // Details:
    std::string mSerial  = {};
    std::string mProduct = {};
    std::string mModel   = {};
    std::string mDevice  = {};

    // Auxiliary:
    std::condition_variable mConnected;

    // Streams:
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

    // Keys:
    std::vector<std::string> mPrivateKeyPaths;
    size_t mLastTriedKey = 0;
    std::string mPublicKeyPath;
    bool mPublicIsAlreadyTried = false;
};

#endif //ADB_LIB_ADBDEVICE_HPP
