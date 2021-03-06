#ifndef ADB_LIB_ADBBASE_HPP
#define ADB_LIB_ADBBASE_HPP

#include "Transport.hpp"
#include "Features.hpp"


class AdbBase {
public:
    using UniquePointer = std::unique_ptr<AdbBase>;
    using SharedPointer = std::shared_ptr<AdbBase>;
    using WeakPointer   = SharedPointer::weak_type;

    using PacketListener = std::function<void (const APacket& /*packet*/)>;
    using ErrorListener  = std::function<void(int /*errorCode*/, const APacket*, bool /*incoming package*/)>;

    using UniqueTransport = Transport::UniquePointer;
    using Arg = uint32_t;

    enum AuthType {
        TOKEN = 1,
        SIGNATURE = 2,
        RSAPUBLICKEY = 3
    };

public: // Manage
    virtual ~AdbBase();

    static SharedPointer makeShared(UniqueTransport&& pointer, uint32_t version = A_VERSION);
    static UniquePointer makeUnique(UniqueTransport&& pointer, uint32_t version = A_VERSION);

    UniqueTransport moveTransportOut(); // Invalidates AdbBase object

public:
    void setVersion(uint32_t version);
    void setMaxData(uint32_t maxData); // overrides default version's maxdata value

    [[nodiscard]] uint32_t getVersion() const;
    [[nodiscard]] uint32_t getMaxData() const;

public: // Util
    [[nodiscard]] bool checkPacketValidity(const APacket& packet) const;

public: // Send
    void sendConnect(const std::string& systemType, const FeatureSet& featureSet);
    void sendTls(Arg type, Arg version);
    void sendAuth(AuthType type, APayload payload);
    void sendOpen(Arg localStreamId, APayload payload);
    void sendReady(Arg localStreamId, Arg remoteStreamId);
    void sendWrite(Arg localStreamId, Arg remoteStreamId, APayload payload);
    void sendClose(Arg localStreamId, Arg remoteStreamId);

    static APayload makeConnectionString(const std::string_view& systemType,
                                         const std::string_view& serial,
                                         const FeatureSet& featureSet);

public: // Incoming packets
    void setPacketListener(PacketListener);
    void resetPacketListener();

public: // Errors
    void setErrorListener(ErrorListener);
    void resetErrorListener();
    void reportSuccessfulSends(bool enable = true);

protected:
    explicit AdbBase(UniqueTransport&& pointer, uint32_t version = A_VERSION);
    AdbBase(AdbBase&& other) noexcept;

    void setup();

private:
    uint32_t mVersion;
    UniqueTransport mTransport;

    PacketListener mPacketListener;
    ErrorListener mErrorListener;
    bool mReportSuccessfulSends;
};


#endif //ADB_LIB_ADBBASE_HPP
