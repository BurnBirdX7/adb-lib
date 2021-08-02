#ifndef ADB_LIB_ADBBASE_HPP
#define ADB_LIB_ADBBASE_HPP

#include "Transport.hpp"
#include "Features.hpp"


class AdbBase {
public:
    using UniquePointer = std::unique_ptr<AdbBase>;
    using SharedPointer = std::shared_ptr<AdbBase>;
    using WeakPointer = SharedPointer::weak_type;

    using UniqueTransport = Transport::UniquePointer;
    using Arg = uint32_t;

    enum AuthType {
        TOKEN = 1,
        SIGNATURE = 2,
        RSAPUBLICKEY = 3
    };

    enum ConnectionState {
        ANY = -1,

        CONNECTING = 0, // No response from the device yet
        AUTHORIZING,    // Sending signed tokens to the device
        UNAUTHORIZED,   // Authorization tokens are exhausted
        NO_PERMISSION,  // Insufficient permissions to connect the device
        OFFLINE,        // ???

        BOOTLOADER,
        DEVICE,
        HOST,
        RECOVERY,
        SIDELOAD,
        RESCUE
    };

public:
    static SharedPointer makeShared(UniqueTransport&& pointer, uint32_t version = A_VERSION);
    static UniquePointer makeUnique(UniqueTransport&& pointer, uint32_t version = A_VERSION);

public:
    UniqueTransport moveTransportOut();
    void setVersion(uint32_t version);
    void setSystemType(const std::string& systemType);
    void setMaxData(uint32_t maxData); // overrides default version's maxdata value
    void setConnectionState(ConnectionState state);

    [[nodiscard]] uint32_t getVersion() const;
    [[nodiscard]] const std::string& getSystemType() const;
    [[nodiscard]] uint32_t getMaxData() const;
    [[nodiscard]] uint32_t getConnectionState() const;

public:
    [[nodiscard]] bool checkPacketValidity(const APacket& packet) const;




public: // send
    void sendConnect(const FeatureSet& featureSet);
    void sendTls(Arg type, Arg version);
    void sendAuth(AuthType type, APayload payload);
    void sendOpen(Arg localStreamId, APayload payload);
    void sendReady(Arg localStreamId, Arg remoteStreamId);
    void sendWrite(Arg localStreamId, Arg remoteStreamId, APayload payload);
    void sendClose(Arg localStreamId, Arg removeStreamId);

protected:
    explicit AdbBase(UniqueTransport&& pointer, uint32_t version = A_VERSION);
    AdbBase(AdbBase&& other) noexcept;

protected:
    UniqueTransport mTransport;

private:
    uint32_t mVersion;
    std::string mSystemType;
    ConnectionState mConnectionState;

};


#endif //ADB_LIB_ADBBASE_HPP
