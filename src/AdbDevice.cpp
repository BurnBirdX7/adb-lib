#include "AdbDevice.hpp"

#include <cassert>
#include <fstream>
#include <filesystem>
#include <iostream>

#include "utils.hpp"
#include "AdbStreams.hpp"


AdbDevice::AdbDevice(AdbDevice::UniqueTransport &&transport)
    : AdbBase(std::move(transport), A_VERSION)
    , mLastLocalId(0)
    , mConnectionState(OFFLINE)
    , mSystemType("none")
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
    mConnected.wait(lock);
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
    else
        std::cerr << "AdbDevice: received unknown command: " << packet.getMessage().viewCommand() << std::endl;

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
    auto activeIt = mActiveStreams.find(localId);
    if (activeIt != mActiveStreams.end()) {
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

    auto activeIt = mActiveStreams.find(localId);
    if (activeIt != mActiveStreams.end()) {
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

    auto it = mActiveStreams.find(localId);
    if (it != mActiveStreams.end()) {
        auto stream = it->second.lock();
        if (stream) {
            stream->received(packet.getPayload());
            sendReady(stream->mLocalId, stream->mRemoteId);
        }
    }
}

void AdbDevice::processAuth(const APacket& packet)
{
    assert(packet.getMessage().command == A_AUTH);
    assert(packet.getMessage().arg0 == AuthType::TOKEN);

    setConnectionState(AUTHORIZING);

    if (!packet.hasPayload()) { // empty AUTH, there's an error, stop authorizing
        setConnectionState(UNAUTHORIZED);
        mConnected.notify_one();
        return;
    }

    if (mNextKey >= mPrivateKeyPaths.size()) {
        if (!sendPublicKey())
            stopConnecting();
        return;
    }

    auto signature = signWithPrivateKey(packet.getPayload());
    if (signature)
        sendAuth(AuthType::SIGNATURE, std::move(*signature));
    else if (!sendPublicKey())
        stopConnecting();
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
    if (errorCode == Transport::ErrorCode::TRANSPORT_DISCONNECTED)
        setConnectionState(ConnectionState::OFFLINE);

    if (errorCode != Transport::ErrorCode::OK)
        ;// TODO: Logging
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
    mActiveStreams[localId] = base;

    mAwaitingStreams.erase(iterator);
    return Streams{AdbIStream(base),
                   AdbOStream(base)};
}

std::shared_ptr<AdbDevice> AdbDevice::make(AdbDevice::UniqueTransport &&transport) {
    return SharedPointer{new AdbDevice{std::move(transport)}};
}

void AdbDevice::closeStream(uint32_t localId)
{
    auto it = mActiveStreams.find(localId);
    if (it == mActiveStreams.end())
        return;

    auto shared = it->second.lock();
    if (shared) {
        sendClose(shared->mLocalId, shared->mRemoteId);
        shared->close();
    }

    mActiveStreams.erase(it);
}

void AdbDevice::send(uint32_t localId, uint32_t remoteId, APayload&& payload)
{
    sendWrite(localId, remoteId, std::move(payload));
}

void AdbDevice::setPrivateKeyPaths(std::vector<std::string> paths)
{
    mPrivateKeyPaths = std::move(paths);
}

void AdbDevice::addPrivateKeyPath(const std::string_view& path)
{
    mPrivateKeyPaths.emplace_back(path);
}

void AdbDevice::setPublicKeyPath(const std::string_view& path)
{
    mPublicKeyPath = path;
}

std::optional<APayload> AdbDevice::signWithPrivateKey(const APayload& hash)
{
    APayload signature(256);
    signature.setDataSize(256);

    bool tokenSigned = false;
    while (!tokenSigned) {
        mbedtls_pk_context* pk = nullptr;
        auto id = mNextKey;
        while (id < mPrivateKeyPaths.size() && !pk) // go through all the keys until we can create pk context
            pk = utils::crypto::makePkContextFromPem(mPrivateKeyPaths[id++]);

        mNextKey = id + 1;

        if (pk == nullptr)  // no valid keys found
            return std::nullopt;

        tokenSigned = utils::crypto::sign(pk,
                                          hash.getBuffer(), hash.getSize(),
                                          signature.getBuffer(), signature.getSize());

        mbedtls_pk_free(pk);
    }

    return signature;
}

bool AdbDevice::sendPublicKey()
{
    if (mPublicKeyPath.empty() || mPublicIsAlreadyTried) {
        stopConnecting();
        return false;
    }

    std::ifstream fin(mPublicKeyPath);
    if (!fin.is_open()) {
        mPublicIsAlreadyTried = true;
        return false;
    }

    size_t size = std::filesystem::file_size(mPublicKeyPath);
    APayload key(size);
    key.setDataSize(size);
    fin.read(reinterpret_cast<char*>(key.getBuffer()), size);
    // TODO: Check I/O exceptions

    sendAuth(AuthType::RSAPUBLICKEY, std::move(key));
    mPublicIsAlreadyTried = true;
    return true;
}

const std::string& AdbDevice::getModel() const
{
    return mModel;
}

const std::string& AdbDevice::getDevice() const
{
    return mDevice;
}

const std::string& AdbDevice::getSerial() const
{
    return mSerial;
}

const std::string& AdbDevice::getProduct() const
{
    return mProduct;
}

bool AdbDevice::setSystemType(const std::string_view& systemType)
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

void AdbDevice::setConnectionState(AdbDevice::ConnectionState state)
{
    mConnectionState = state;
}

const std::string& AdbDevice::getSystemType() const
{
    return mSystemType;
}

uint32_t AdbDevice::getConnectionState() const
{
    return mConnectionState;
}

const FeatureSet& AdbDevice::getFeatures() const
{
    return mFeatureSet;
}

void AdbDevice::stopConnecting()
{
    setConnectionState(UNAUTHORIZED);
    mConnected.notify_one();
}
