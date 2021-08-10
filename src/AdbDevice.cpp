#include "AdbDevice.hpp"

#include <cassert>

#include "utils.hpp"
#include "AdbStreams.hpp"


AdbDevice::AdbDevice(AdbDevice::UniqueTransport &&transport)
    : AdbBase(std::move(transport), A_VERSION)
    , mLastLocalId(0)
{
    setPacketListener([this](const APacket& packet){
        this->packetListener(packet);
    });

    setErrorListener([this](int errorCode, const APacket* packet, bool incomingPacket) {
        this->errorListener(errorCode, packet, incomingPacket);
    });
}

void AdbDevice::connect() {
    assert(getConnectionState() == OFFLINE);

    sendConnect("host", mFeatureSet);
    setConnectionState(CONNECTING);

    // TODO: Replace with something sane
    std::mutex mutex;
    std::unique_lock lock(mutex);
    mConnected.wait_for(lock, std::chrono::seconds(5));
    if (!isConnected())
        setConnectionState(OFFLINE);
}

bool AdbDevice::isConnected() const {
    switch(getConnectionState()) {
        case BOOTLOADER:
        case DEVICE:
        case HOST:
        case RECOVERY:
        case SIDELOAD:
        case RESCUE:
            return true;
        default:
            return false;
    }
}

void AdbDevice::setFeatures(FeatureSet featureSet) {
    mFeatureSet = std::move(featureSet);
}

void AdbDevice::packetListener(const APacket &packet) {
    auto command = packet.getMessage().command;

    if (command == A_CNXN)
        processConnect(packet);
    else if (command == A_OPEN)
        processOpen(packet);
    else if (command == A_OKAY)
        processReady(packet);
    else if (command == A_CLSE)
        processClose(packet);
    else if (command == A_WRTE)
        processWrite(packet);
    else if (command == A_AUTH)
        processAuth(packet);
    else if (command == A_STLS)
        processTls(packet);
}

bool AdbDevice::isAwaitingConnection() const
{
    switch (getConnectionState()) {
        case CONNECTING:
        case AUTHORIZING:
        case UNAUTHORIZED:
            return true;
        default:
            return false;
    }
}

void AdbDevice::processConnect(const APacket& packet) {
    assert(packet.getMessage().command == A_CNXN && packet.hasPayload());

    // Version and
    setVersion(std::min(packet.getMessage().arg0, getVersion()));
    setMaxData(std::min(packet.getMessage().arg1, getMaxData()));

    if (isAwaitingConnection()) {
        auto view = packet.getPayload().toStringView();
        auto tokens = utils::tokenize(view, ":");

        if (!setSystemType(tokens[0])) {
            setConnectionState(OFFLINE);
            return;
        }

        mSerial = std::string(tokens[1]);

        if (tokens.size() > 2) {
            auto properties = utils::tokenize(tokens[2], ";");
            for(const auto property : properties) {
                auto key_value = utils::tokenize(property, "=");
                const auto& key = key_value[0];
                const auto& value = key_value[1];

                if (key == "features")
                    mFeatureSet = Features::stringToSet(value);
                else if (key == "ro.product.name")
                    mProduct = value;
                else if (key == "ro.product.model")
                    mModel = value;
                else if (key == "ro.product.device")
                    mDevice = value;

                // TODO: Report unknown property (?)
            }
        }
        mConnected.notify_one();
    } // !isAwaitingConnection()
    // TODO: Else
}

// TODO: Process Packets
void AdbDevice::processOpen(const APacket&)
{

}

void AdbDevice::processReady(const APacket& packet)
{
    assert(packet.getMessage().command == A_OKAY);
    const auto& message = packet.getMessage();
    auto localId = message.arg1;

    // find if it is an active stream
    auto activeIt = mStreams.find(localId);
    if (activeIt != mStreams.end()) {
        auto stream = activeIt->second.lock();
        if (stream)
            stream->readyToSend();
        return;
    }

    auto awaitingIt = mAwaitingStreams.find(localId);
    if (awaitingIt != mAwaitingStreams.end()) {
        awaitingIt->second.remoteId = message.arg0;
        awaitingIt->second.cv.notify_one();
    }
}

void AdbDevice::processClose(const APacket& packet)
{
    const auto& message = packet.getMessage();
    assert(message.command == A_CLSE);

    auto localId = message.arg1;
    std::unique_lock lock(mStreamsMutex);

    auto awaitingIt = mAwaitingStreams.find(localId);
    if (awaitingIt != mAwaitingStreams.end()) {
        auto& awaitingStruct = awaitingIt->second;
        awaitingStruct.rejected = true;
        awaitingStruct.remoteId = 0;
        awaitingStruct.cv.notify_one();
        return;
    }

    auto activeIt = mStreams.find(localId);
    if (activeIt != mStreams.end()) {
        auto shared = activeIt->second.lock();
        if (shared)
            shared->close();
    }
}

void AdbDevice::processWrite(const APacket& packet)
{
    assert(packet.getMessage().command == A_WRTE);
    if (!packet.hasPayload())
        return;

    const auto& message = packet.getMessage();
    auto localId = message.arg1;

    auto it = mStreams.find(localId);
    if (it != mStreams.end()) {
        auto stream = it->second.lock();
        if (stream)
            stream->received(packet.getPayload());
    }
}

void AdbDevice::processAuth(const APacket& packet)
{

}

void AdbDevice::processTls(const APacket&)
{

}

AdbDevice::~AdbDevice()
{
    resetPacketListener();
    resetErrorListener();
}

void AdbDevice::errorListener(int errorCode, const APacket* packet, bool incomingPacket)
{
    // TODO: Process Errors
}

std::optional<AdbDevice::Streams>  AdbDevice::open(const std::string_view& destination)
{
    std::unique_lock lock (mStreamsMutex);
    auto localId = ++mLastLocalId;
    auto pairIteratorBool = mAwaitingStreams.emplace(std::piecewise_construct,
                                                     std::make_tuple(localId),
                                                     std::make_tuple());

    if (!pairIteratorBool.second)
        return std::nullopt;

    auto& iterator = pairIteratorBool.first;
    auto& awaitingStruct = iterator->second;

    AdbBase::sendOpen(localId, std::move(APayload(destination)));

    // Wait for READY or CLOSE packet
    awaitingStruct.cv.wait(lock);
    if (awaitingStruct.rejected) {
        mAwaitingStreams.erase(iterator);
        return std::nullopt;
    }

    std::shared_ptr<AdbStreamBase> base{new AdbStreamBase{shared_from_this(), localId, awaitingStruct.remoteId}};
    mStreams[localId] = base;

    mAwaitingStreams.erase(iterator);
    return Streams{AdbIStream(base),
                   AdbOStream(base)};
}

std::shared_ptr<AdbDevice> AdbDevice::make(AdbDevice::UniqueTransport &&transport) {
    return SharedPointer{new AdbDevice{std::move(transport)}};
}

void AdbDevice::closeStream(uint32_t localId)
{
    auto it = mStreams.find(localId);
    if (it == mStreams.end())
        return;

    auto shared = it->second.lock();
    if (shared) {
        sendClose(shared->mLocalId, shared->mRemoteId);
        shared->close();
    }

    mStreams.erase(it);
}

void AdbDevice::send(uint32_t localId, uint32_t remoteId, APayload&& payload)
{
    sendWrite(localId, remoteId, std::move(payload));
}
