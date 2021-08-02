#include "AdbDevice.hpp"

#include <cassert>
#include <sstream>

AdbDevice::AdbDevice(AdbDevice::UniqueTransport &&transport)
    : AdbBase(std::move(transport), A_VERSION)
{
    mTransport->setReceiveListener(
            [this](const APacket* packet, Transport::ErrorCode code) {
                // TODO: Process Error
                if (code == Transport::OK)
                    this->receiveListener(*packet);
            }
    );

    mTransport->setSendListener(
            [this] (const APacket* packet, Transport::ErrorCode code) {
                // TODO: Process Error
            }
    );
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

void AdbDevice::receiveListener(const APacket &packet) {
    switch(packet.getMessage().command) {
        case A_CNXN:
            processConnect(packet);
            break;

        // TODO: Other cases
    }

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

    // TODO: Make split via search and not a string stream
    auto view = packet.getPayload().toStringView();
    std::stringstream ss;
    ss << view;

    std::vector<std::string> tokens;
    for (std::string part; getline(ss, part, ':'); /* --- */)
        tokens.push_back(part);

    // TODO: Check vector size

    std::string& type = tokens[0];
    ConnectionState newState;
    if(type == "bootloader")
        newState = BOOTLOADER;
    else if (type == "device")
        newState = DEVICE;
    else if (type == "host")
        newState = HOST;
    else if (type == "recovery")
        newState = RECOVERY;
    else if (type == "sideload")
        newState = SIDELOAD;
    else if (type == "rescue")
        newState = RESCUE;
    else
        newState = ANY; // TODO: Process exception

    setConnectionState(newState);
}
