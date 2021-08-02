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
    assert(packet.getMessage().command == A_CNXN);

    // TODO: Function's Body
}
