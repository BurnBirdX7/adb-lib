#include "AdbDevice.hpp"

#include <cassert>

#include "utils.hpp"
#include "AStream.hpp"


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
    } // !isAwaitingConnection()
    // TODO: Else
}

// TODO: Process Packets
void AdbDevice::processOpen(const APacket&)
{

}

void AdbDevice::processReady(const APacket&)
{

}

void AdbDevice::processClose(const APacket&)
{

}

void AdbDevice::processWrite(const APacket&)
{

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

std::optional<AdbDevice::StreamsRef>  AdbDevice::open(const std::string_view& destination)
{
    std::unique_lock lock (mStreamsMutex);
    auto myId = ++mLastLocalId;
    auto pairIteratorBool = mAwaitingStreams.emplace(std::piecewise_construct,
                                                     std::make_tuple(myId),
                                                     std::make_tuple());

    if (!pairIteratorBool.second)
        return std::nullopt;

    auto& iterator = pairIteratorBool.first;
    auto& awaitingStruct = iterator->second; // pair.iterator->mapValue

    AdbBase::sendOpen(myId, std::move(APayload(destination)));
    // Wait for READY or CLOSE packet
    awaitingStruct.cv.wait(lock);

    if (awaitingStruct.rejected) {
        mAwaitingStreams.erase(iterator);
        return std::nullopt;
    }

    auto& streams = mStreams[myId];
    streams.istream.reset(new AIStream{shared_from_this()});
    streams.ostream.reset(new AOStream{shared_from_this(), myId, awaitingStruct.remoteId});

    mAwaitingStreams.erase(iterator);
    return StreamsRef{*streams.istream, *streams.ostream};
}

std::shared_ptr<AdbDevice> AdbDevice::make(AdbDevice::UniqueTransport &&transport) {
    return SharedPointer{new AdbDevice{std::move(transport)}};
}
