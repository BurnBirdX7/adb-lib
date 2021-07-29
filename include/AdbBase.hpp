#ifndef ADB_LIB_ADBBASE_HPP
#define ADB_LIB_ADBBASE_HPP

#include "Transport.hpp"
#include "Features.hpp"


class AdbBase {
public:
    using UniqueTransport = Transport::UniquePointer;
    using Arg = uint32_t;

    enum AuthType {
        TOKEN = 1,
        SIGNATURE = 2,
        RSAPUBLICKEY = 3
    };

public:
    explicit AdbBase(UniqueTransport&& pointer, uint32_t version = A_VERSION);
    AdbBase(AdbBase&& other) noexcept ;

public:
    UniqueTransport moveTransportOut();
    void setVersion(uint32_t version);
    void setSystemType(const std::string& systemType);
    void setMaxData(uint32_t maxData); // overrides default version's maxdata value

    [[nodiscard]] uint32_t getVersion() const;
    [[nodiscard]] const std::string& getSystemType() const;
    [[nodiscard]] uint32_t getMaxData() const;

public: // send
    void sendConnect(const FeatureSet& featureSet);
    void sendTls(Arg type, Arg version);
    void sendAuth(AuthType type, APayload payload);
    void sendOpen(Arg localStreamId, APayload payload);
    void sendReady(Arg localStreamId, Arg remoteStreamId);
    void sendClose(Arg localStreamId, Arg removeStreamId);

private:
    UniqueTransport mTransport;
    uint32_t mVersion;
    std::string mSystemType;

};


#endif //ADB_LIB_ADBBASE_HPP
