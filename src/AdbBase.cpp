#include "AdbBase.hpp"



AdbBase::UniqueTransport AdbBase::moveTransportOut()
{
    return std::move(mTransport);
}

void AdbBase::setVersion(uint32_t version)
{
    mVersion = version;
    if (mVersion < A_VERSION_MIN)   // for obsolete versions
        mTransport->setMaxPayloadSize(MAX_PAYLOAD_V1);
}



void AdbBase::setMaxData(uint32_t maxData)
{
    mTransport->setMaxPayloadSize(maxData);
}

uint32_t AdbBase::getVersion() const
{
    return mVersion;
}

uint32_t AdbBase::getMaxData() const
{
    return mTransport->getMaxPayloadSize();
}

bool AdbBase::checkPacketValidity(const APacket& packet) const
{
    const auto& message = packet.getMessage();
    if (packet.hasPayload()) {

        const auto& payload = packet.getPayload();
        if (payload.getSize() != message.dataLength)
            return false;

        if (mVersion < A_VERSION_SKIP_CHECKSUM) {
            uint32_t checksum = 0;
            for (const auto& byte : payload)
                checksum += byte;
            if (checksum != message.dataCheck)
                return false;
        }

    }
    else if (message.dataCheck != 0 || message.dataLength != 0)
        return false;

    return true;
}

void AdbBase::sendConnect(const std::string& systemType, const FeatureSet& featureSet)
{
    std::string identity = systemType + "::"; // TODO: Add possibility to add Serial number to the identity string
    identity += "features=" + Features::setToString(featureSet);

    APacket packet(AMessage::make(A_CNXN, mVersion, mTransport->getMaxPayloadSize()));
    packet.movePayloadIn(APayload(identity));
    packet.updateMessageDataLength();
    packet.computeChecksum(); // Checksum has to be computed regardless of version

    mTransport->send(std::move(packet));
}

void AdbBase::sendTls(AdbBase::Arg type, AdbBase::Arg version)
{
    mTransport->send(APacket(AMessage::make(A_STLS, type, version)));
}

void AdbBase::sendAuth(AdbBase::AuthType type, APayload payload)
{
    APacket packet(AMessage::make(A_AUTH, type, 0));
    packet.movePayloadIn(std::move(payload));
    packet.updateMessageDataLength();
    packet.computeChecksum(); // Checksum has to be computed regardless of version
    mTransport->send(std::move(packet));
}

void AdbBase::sendOpen(AdbBase::Arg localStreamId, APayload payload)
{
    APacket packet(AMessage::make(A_OPEN, localStreamId, 0));
    packet.movePayloadIn(std::move(payload));
    packet.updateMessageDataLength();

    if (mVersion <= A_VERSION_SKIP_CHECKSUM)
        packet.computeChecksum();

    mTransport->send(std::move(packet));
}

void AdbBase::sendReady(AdbBase::Arg localStreamId, AdbBase::Arg remoteStreamId)
{
    mTransport->send(APacket(AMessage::make(A_OKAY, localStreamId, remoteStreamId)));
}

void AdbBase::sendWrite(AdbBase::Arg localStreamId, AdbBase::Arg remoteStreamId, APayload payload)
{
    APacket packet(AMessage::make(A_WRTE, localStreamId, remoteStreamId));
    packet.movePayloadIn(std::move(payload));
    packet.updateMessageDataLength();

    if (mVersion < A_VERSION_SKIP_CHECKSUM)
        packet.computeChecksum();

    mTransport->send(std::move(packet));
}

void AdbBase::sendClose(AdbBase::Arg localStreamId, AdbBase::Arg remoteStreamId)
{
    mTransport->send(APacket(AMessage::make(A_CLSE, localStreamId, remoteStreamId)));
}

AdbBase::~AdbBase()
{
    mTransport->resetReceiveListener();
    mTransport->resetSendListener();
}

AdbBase::SharedPointer AdbBase::makeShared(AdbBase::UniqueTransport &&pointer, uint32_t version) {
    return SharedPointer(new AdbBase{std::move(pointer), version});
}

AdbBase::UniquePointer AdbBase::makeUnique(AdbBase::UniqueTransport &&pointer, uint32_t version) {
    return UniquePointer(new AdbBase{std::move(pointer), version});
}

void AdbBase::setPacketListener(AdbBase::PacketListener listener)
{
    mPacketListener = std::move(listener);
}

void AdbBase::resetPacketListener()
{
    mPacketListener = {};
}

void AdbBase::setErrorListener(AdbBase::ErrorListener listener)
{
    mErrorListener = std::move(listener);
}

void AdbBase::resetErrorListener()
{
    mErrorListener = {};
}

AdbBase::AdbBase(AdbBase::UniqueTransport&& pointer, uint32_t version)
        : mTransport(std::move(pointer))
        , mVersion(0)
{
    setVersion(version);
    setup();
}

AdbBase::AdbBase(AdbBase&& other) noexcept
        : mTransport(std::move(other.mTransport))
        , mVersion(other.mVersion)
        , mReportSuccessfulSends(false)
{
    setup();
}

void AdbBase::setup()
{
    mTransport->setReceiveListener(
            [this](const APacket* packet, Transport::ErrorCode errorCode) {
                if (errorCode == Transport::OK && mPacketListener)
                    mPacketListener(*packet);
                else if(mErrorListener)
                    mErrorListener(errorCode, packet, true);

                mTransport->receive();
            }
    );

    mTransport->setSendListener(
            [this](const APacket* packet, Transport::ErrorCode errorCode) {
                if ((errorCode != Transport::OK || mReportSuccessfulSends) && mErrorListener)
                    mErrorListener(errorCode, packet, false);
            }
    );

    mTransport->receive();
}

APayload AdbBase::makeConnectionString(const std::string_view& systemType,
                                       const std::string_view& serial,
                                       const FeatureSet& featureSet)
{
    auto featureString = "features=" + Features::setToString(featureSet);
    size_t len = systemType.length() + serial.length() + featureString.length() + 2;
    APayload payload(len);
    payload.setDataSize(len);
    size_t payloadI = 0;

    for (size_t i = 0; i < systemType.size(); ++i, ++payloadI)
        payload[payloadI] = systemType[i];
    payload[++payloadI] = ':';

    for (size_t i = 0; i < serial.size(); ++i, ++payloadI)
        payload[payloadI] = serial[i];
    payload[++payloadI] = ':';

    for (size_t i = 0; i < featureString.size(); ++i, ++payloadI)
        payload[payloadI] = featureString[i];

    return payload;
}

void AdbBase::reportSuccessfulSends(bool enable)
{
    mReportSuccessfulSends = enable;
}
