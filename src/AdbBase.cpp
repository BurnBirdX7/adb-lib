#include "AdbBase.hpp"


AdbBase::AdbBase(AdbBase::UniqueTransport&& pointer, uint32_t version)
    : mTransport(std::move(pointer))
    , mVersion(version)
    , mSystemType()
{}

AdbBase::AdbBase(AdbBase&& other) noexcept
    : mTransport(std::move(other.mTransport))
    , mVersion(other.mVersion)
    , mSystemType(std::move(other.mSystemType))
{}

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

void AdbBase::setSystemType(const std::string& systemType)
{
    mSystemType = systemType;
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
