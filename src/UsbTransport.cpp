#include "UsbTransport.hpp"

#include <cassert>
#include <iostream>
#include <tuple>

#include <LibusbError.hpp>


UsbTransport::UsbTransport(const LibusbDevice& device, const InterfaceData& interfaceData)
    : mDevice(device.referenceDevice())
    , mHandle(device.open())
    , mInterfaceData(interfaceData)
    , mFlags(TRANSPORT_IS_OK)
{
    try {
        mHandle.claimInterface(mInterfaceData.interfaceNumber);
        mFlags |= INTERFACE_CLAIMED;

        mHandle.clearHalt(mInterfaceData.writeEndpointAddress);
        mHandle.clearHalt(mInterfaceData.readEndpointAddress);
    }
    catch (LibusbError& err) {
        OBJLIBUSB_IOSTREAM_REPORT_ERROR(std::cerr, err);
        mFlags &= ~TRANSPORT_IS_OK;
    }

    prepareReceivePacket();
    prepareReceiveTransfers();
}

std::optional<InterfaceData> UsbTransport::findAdbInterface(const LibusbDevice& device)
{
    // TODO: Logging
    auto deviceDescriptor = device.getDescriptor();
    if (deviceDescriptor->bDeviceClass != LIBUSB_CLASS_PER_INTERFACE)
        return {}; // Skip device with incorrect class

    auto configDescriptor = device.getActiveConfigDescriptor();
    const size_t interfaceCount = configDescriptor->bNumInterfaces;
    InterfaceData data{};
    bool found = false;
    int interfaceNumber;

    // Run through interfaces of the device
    for (interfaceNumber = 0; !found && interfaceNumber < interfaceCount; ++interfaceNumber) {
        data.interfaceNumber = interfaceNumber;

        const auto& interface = configDescriptor->interface[interfaceNumber];
        if (interface.num_altsetting != 1)
            continue; // skip the interface if zero altsetting OR more than one

        const auto& interfaceDescriptor = interface.altsetting[0];

        if (!isAdbInterface(interfaceDescriptor))
            continue; // skipping non-adb interface

        bool found_in = false;
        bool found_out = false;
        const int endpointCount = interfaceDescriptor.bNumEndpoints;

        // Run through endpoints of the interface
        for (int endpointNumber = 0; endpointNumber < endpointCount; ++endpointNumber) {
            const auto& endpointDescriptor = interfaceDescriptor.endpoint[endpointNumber];
            const uint8_t endpointAddress = endpointDescriptor.bEndpointAddress;
            const uint8_t endpointAttribute = endpointDescriptor.bmAttributes;
            const uint8_t transferType = endpointAttribute & LIBUSB_TRANSFER_TYPE_MASK;

            if (transferType != LIBUSB_TRANSFER_TYPE_BULK)
                continue; // skip if transfer type is not bulk

            if (isEndpointOutput(endpointAddress) && !found_out) {
                found_out = true;
                data.writeEndpointAddress = endpointAddress;
                data.zeroMask = endpointDescriptor.wMaxPacketSize - 1;
            }
            else if (!isEndpointOutput(endpointAddress) && !found_in) {
                found_in = true;
                data.readEndpointAddress = endpointAddress;
            }

            size_t endpointPacketSize = endpointDescriptor.wMaxPacketSize;
            assert(endpointPacketSize != 0);
            if (data.packetSize == 0)
                data.packetSize = endpointPacketSize;
            else
                assert(data.packetSize == endpointPacketSize);
        } // ! Run through endpoints

        found = found_in && found_out;
    } // Run through interfaces

    if (!found)
        return {};
    return data;
}

UsbTransport::UsbTransport(UsbTransport&& other) noexcept
        : mDevice(std::move(other.mDevice))
        , mHandle(std::move(other.mHandle))
        , mInterfaceData(other.mInterfaceData)
        , mFlags(other.mFlags)
{
    other.mFlags = 0; // other is NOT ok and interface is NOT claimed

    prepareReceiveTransfers();
}

UsbTransport::~UsbTransport()
{
    if ((mFlags & INTERFACE_CLAIMED) == INTERFACE_CLAIMED)
        mHandle.releaseInterface(mInterfaceData.interfaceNumber);
}

std::optional<UsbTransport> UsbTransport::createTransport(const LibusbDevice& device)
{
    auto interfaceData = findAdbInterface(device);
    if (!interfaceData.has_value())
        return {}; // If interface wasn't found - return empty pointer

    UsbTransport transport(device, interfaceData.value());
    if (transport.isOk())
        return transport;

    return {};
}

bool UsbTransport::isAdbInterface(const libusb_interface_descriptor& interfaceDescriptor)
{
    const int usb_class = interfaceDescriptor.bInterfaceClass;
    const int usb_subclass = interfaceDescriptor.bInterfaceSubClass;
    const int usb_protocol = interfaceDescriptor.bInterfaceProtocol;
    return (usb_class == ADB_CLASS) &&
           (usb_subclass == ADB_SUBCLASS) &&
           (usb_protocol == ADB_PROTOCOL);
}

bool UsbTransport::isEndpointOutput(uint8_t endpointAddress)
{
    return (endpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT;
}

bool UsbTransport::isOk() const
{
    return (mFlags & TRANSPORT_IS_OK) == TRANSPORT_IS_OK;
}
void UsbTransport::send(APacket&& packet)
{
    std::scoped_lock lock(mSendMutex);

    static size_t transferId = 0;
    auto res = mSendTransfers.emplace(transferId++, std::move(packet));
    auto& transfers = res.first->second; // res.[iterator]->[value]

    auto* callbackData = new CallbackData{.transport = this,            // Has to be deleted in a callback
                                          .transferId = transferId};    // (either message's or payload's)
    auto& message = transfers.packet.getMessage();
    transfers.messageTransfer = LibusbTransfer::createTransfer();
    transfers.messageTransfer->fillBulk(mHandle,
                                        mInterfaceData.writeEndpointAddress,
                                        reinterpret_cast<uint8_t*>(&message),
                                        sizeof(AMessage),
                                        sSendHeadCallback,
                                        callbackData,
                                        0);
    transfers.messageTransfer->submit();

    if (transfers.packet.hasPayload()) {
        auto& payload = transfers.packet.getPayload();
        transfers.payloadTransfer = LibusbTransfer::createTransfer();
        transfers.payloadTransfer->fillBulk(mHandle,
                                            mInterfaceData.writeEndpointAddress,
                                            payload.getBuffer(),
                                            payload.getSize(),
                                            sSendPayloadCallback,
                                            callbackData,
                                            0);
        transfers.payloadTransfer->submit();
    }
}

void UsbTransport::receive()
{
    std::scoped_lock lock(mReceiveMutex);
    if (isReceiving)
        return;

    // transfers should be already prepared
    mReceiveTransfer.messageTransfer->submit();

}

void UsbTransport::sSendHeadCallback(const LibusbTransfer::Pointer& headTransfer)
{
    // GET ESSENTIAL DATA:
    auto callbackData = static_cast<CallbackData*>(headTransfer->getUserData());
    auto* transport = callbackData->transport;
    auto transferId = callbackData->transferId;

    // FIND TRANSFERS DATA:
    std::scoped_lock lock(transport->mSendMutex);
    auto transfers = transport->mSendTransfers.find(transferId);
    if (transfers == transport->mSendTransfers.end()) {
        transport->finishSendTransfer(callbackData, transfers);
        return;
    } // !

    auto& transfer = transfers->second;
    transfer.errorCode = getTransferStatus(headTransfer->getStatus());

    if (transfer.payloadTransfer == nullptr)    // if there's no payload, we finish transfer
        transport->finishSendTransfer(callbackData, transfers);
    else if (transfer.errorCode != OK)          // if there's a payload and head transfer failed
        transfer.payloadTransfer->cancel();
    // else // if there's a payload and transfer is complete
}

void UsbTransport::sSendPayloadCallback(const LibusbTransfer::Pointer& payloadTransfer)
{
    auto callbackData = static_cast<CallbackData*>(payloadTransfer->getUserData());
    auto* transport = callbackData->transport;
    auto transferId = callbackData->transferId;

    // FIND TRANSFERS DATA:
    std::scoped_lock lock(transport->mSendMutex);
    auto transfers = transport->mSendTransfers.find(transferId);
    if (transfers == transport->mSendTransfers.end()) {
        transport->finishSendTransfer(callbackData, transfers);
        return;
    } // !

    ErrorCode ec = getTransferStatus(payloadTransfer->getStatus());
    if (ec != CANCELLED || transfers->second.errorCode == OK)   // if this transfer wasn't cancelled or
        transfers->second.errorCode = ec;                       // if previous transfer is ok, report this transfer's error code

    transport->finishSendTransfer(callbackData, transfers);
}

void UsbTransport::sReceiveHeadCallback(const LibusbTransfer::Pointer& headTransfer)
{
    // GET ESSENTIAL DATA:
    auto* transport = static_cast<UsbTransport*>(headTransfer->getUserData());
    auto& transfer = transport->mReceiveTransfer;
    auto& packet = transfer.packet;

    std::scoped_lock lock(transport->mReceiveMutex);
    transfer.errorCode = getTransferStatus(headTransfer->getStatus());

    if (transfer.errorCode == OK && packet.getMessage().dataLength != 0)
        transfer.payloadTransfer->submit();
    else    // if there's error or dataLength equals zero
        transport->finishReceiveTransfer();

}

void UsbTransport::sReceivePayloadCallback(const LibusbTransfer::Pointer& payloadTransfer)
{
    // GET ESSENTIAL DATA:
    auto* transport = static_cast<UsbTransport*>(payloadTransfer->getUserData());
    auto& transfer = transport->mReceiveTransfer;
    auto& packet = transfer.packet;

    std::scoped_lock lock(transport->mReceiveMutex);
    transfer.errorCode = getTransferStatus(payloadTransfer->getStatus());

    // Doesn't matter if there's an error or not
    packet.getPayload().setDataSize(payloadTransfer->getActualLength());
    transport->finishReceiveTransfer();
}

Transport::ErrorCode UsbTransport::getTransferStatus(int status) {
    if (status == LIBUSB_TRANSFER_COMPLETED)
        return ErrorCode::OK;
    else if (status == LIBUSB_TRANSFER_CANCELLED)
        return ErrorCode::CANCELLED;
    else
        return ErrorCode::UNDERLYING_ERROR;
}

const AMessage& UsbTransport::messageFromBuffer(const uint8_t* buffer) {
    return *reinterpret_cast<const AMessage*>(buffer);
}

void UsbTransport::prepareReceivePacket() {
    std::scoped_lock lock(mReceiveMutex);
    auto& packet = mReceiveTransfer.packet;

    if (!packet.hasPayload())
        packet.movePayloadIn(APayload{mMaxPayloadSize});
    else
        packet.getPayload().resizeBuffer(mMaxPayloadSize);


}

void UsbTransport::setVersion(uint32_t version) {
    Transport::setVersion(version);
    prepareReceivePacket();
    prepareReceiveTransfers();
}

void UsbTransport::prepareReceiveTransfers() {
    // Transfers point to the exact location of UsbTransport object
    // This function MUST be called on move operations
    auto headBuffer = reinterpret_cast<unsigned char*>(&mReceiveTransfer.packet.getMessage());
    auto headBufferSize = sizeof(AMessage);
    auto headTransfer = LibusbTransfer::createTransfer();
    headTransfer->fillBulk(mHandle,
                           mInterfaceData.readEndpointAddress,
                           headBuffer,
                           headBufferSize,
                           sReceiveHeadCallback,
                           this,
                           0);

    auto payloadBuffer = mReceiveTransfer.packet.getPayload().getBuffer();
    auto payloadBufferSize = mReceiveTransfer.packet.getPayload().getBufferSize();
    auto payloadTransfer = LibusbTransfer::createTransfer();
    payloadTransfer->fillBulk(mHandle,
                              mInterfaceData.readEndpointAddress,
                              payloadBuffer,
                              payloadBufferSize,
                              sReceivePayloadCallback,
                              this,
                              0);
}

void UsbTransport::finishSendTransfer(UsbTransport::CallbackData* callbackData,
                                      UsbTransport::TransfersContainer::iterator mapIterator)
{
    delete callbackData;

    const APacket* packet;
    ErrorCode ec;
    if (mapIterator == mSendTransfers.end()) {
        packet = nullptr;
        ec = TRANSPORT_ERROR;
    }
    else {
        packet = &mapIterator->second.packet;
        ec = mapIterator->second.errorCode;
    }

    mSendTransfers.erase(mapIterator);
    notifySendListener(packet, ec);
}

void UsbTransport::finishReceiveTransfer() {
    notifyReceiveListener(&mReceiveTransfer.packet, mReceiveTransfer.errorCode);
    isReceiving = false;
}
