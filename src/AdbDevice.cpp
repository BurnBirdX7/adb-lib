#include "AdbDevice.hpp"

#include <cassert>
#include <utils.hpp>


AdbDevice::AdbDevice(AdbDevice::UniqueTransport &&transport)
    : AdbBase(std::move(transport), A_VERSION)
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
    sendConnect(mFeatureSet);
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

    /*
     * if (isAwaitingConnection())
     *   should be here
     *
     * TODO: Make CNXN processing for a situation when we are not the initiator
     */

    // Version and
    setVersion(std::min(packet.getMessage().arg0, getVersion()));
    setMaxData(std::min(packet.getMessage().arg1, getMaxData()));

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
