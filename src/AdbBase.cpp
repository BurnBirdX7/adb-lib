#include "AdbBase.hpp"



AdbBase::UniqueTransport AdbBase::moveTransportOut()
{
    setConnectionState(ConnectionState::ANY);
    return std::move(mTransport);
}

void AdbBase::setVersion(uint32_t version)
{
    mVersion = version;
    if (mVersion < A_VERSION_MIN)   // for obsolete versions
        mTransport->setMaxPayloadSize(MAX_PAYLOAD_V1);
}

bool AdbBase::setSystemType(const std::string_view& systemType)
{
    ConnectionState newState;
    if(systemType == "bootloader")
        newState = BOOTLOADER;
    else if (systemType == "device")
        newState = DEVICE;
    else if (systemType == "host")
        newState = HOST;
    else if (systemType == "recovery")
        newState = RECOVERY;
    else if (systemType == "sideload")
        newState = SIDELOAD;
    else if (systemType == "rescue")
        newState = RESCUE;
    else
        return false;

    setConnectionState(newState);
    mSystemType = systemType;
    return true;
}

void AdbBase::setMaxData(uint32_t maxData)
{
    mTransport->setMaxPayloadSize(maxData);
}

void AdbBase::setConnectionState(AdbBase::ConnectionState state)
{
    mConnectionState = state;
}

uint32_t AdbBase::getVersion() const
{
    return mVersion;
}

const std::string& AdbBase::getSystemType() const
{
    return mSystemType;
}

uint32_t AdbBase::getMaxData() const
{
    return mTransport->getMaxPayloadSize();
}

uint32_t AdbBase::getConnectionState() const
{
    return mConnectionState;
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

void AdbBase::sendConnect(const FeatureSet& featureSet)
{
    APacket packet(AMessage::make(A_CNXN, mVersion, mTransport->getMaxPayloadSize()));
    std::string identity = mSystemType + "::"; // TODO: Add possibility to add Serial number to the identity string
    identity += "features=" + Features::setToString(featureSet);

    APayload payload(identity.size());
    std::copy(identity.begin(), identity.end(), payload.getBuffer());

    packet.movePayloadIn(std::move(payload));
    packet.updateMessageDataLength();
    packet.computeChecksum();               // Checksum has to be computed regardless of version

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
    packet.computeChecksum();
    mTransport->send(std::move(packet));
}

void AdbBase::sendOpen(AdbBase::Arg localStreamId, APayload payload)
{
    APacket packet(AMessage::make(A_OPEN, localStreamId, 0));
    packet.movePayloadIn(std::move(payload));
    packet.updateMessageDataLength();
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

void AdbBase::sendClose(AdbBase::Arg localStreamId, AdbBase::Arg removeStreamId)
{
    mTransport->send(APacket(AMessage::make(A_CLSE, localStreamId, removeStreamId)));
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
        , mSystemType()
        , mConnectionState(OFFLINE)
{
    setVersion(version);
    listenersSetup();
}

AdbBase::AdbBase(AdbBase&& other) noexcept
        : mTransport(std::move(other.mTransport))
        , mVersion(other.mVersion)
        , mSystemType(std::move(other.mSystemType))
        , mConnectionState(OFFLINE)
{
    listenersSetup();
}

void AdbBase::listenersSetup()
{
    mTransport->setReceiveListener(
            [this](const APacket* packet, Transport::ErrorCode errorCode) {
                if (errorCode == Transport::OK) {
                    if (mPacketListener)
                        mPacketListener(*packet);
                    return;
                }

                if(mErrorListener)
                    mErrorListener(errorCode, packet, false);
            }
    );

    mTransport->setSendListener(
            [this](const APacket* packet, Transport::ErrorCode errorCode) {
                if (errorCode != Transport::OK && mErrorListener)
                    mErrorListener(errorCode, packet, true);
            }
    );
}
