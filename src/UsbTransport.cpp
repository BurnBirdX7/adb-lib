#include "UsbTransport.hpp"

#include <cassert>
#include <iostream>
#include <tuple>

#include <ObjLibusb-1.0/ObjLibusb/Error.hpp>


UsbTransport::UsbTransport(const Device& device, const InterfaceData& interfaceData)
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
    catch (ObjLibusbError& err) {
        std::cerr << "UsbTransfer caught exception: " << err.what() << std::endl;
        mFlags &= ~TRANSPORT_IS_OK;
    }
}

std::optional<InterfaceData> UsbTransport::findAdbInterface(const Device& device)
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
}

UsbTransport::~UsbTransport()
{
    if ((mFlags & INTERFACE_CLAIMED) == INTERFACE_CLAIMED)
        mHandle.releaseInterface(mInterfaceData.interfaceNumber);
}

std::unique_ptr<UsbTransport> UsbTransport::make(const Device& device)
{
    auto interfaceData = findAdbInterface(device);
    if (!interfaceData.has_value())
        return {}; // If interface wasn't found - return empty pointer

    return make(device, *interfaceData);
}

std::unique_ptr<UsbTransport>
UsbTransport::make(const Device& device, const InterfaceData& interfaceHint)
{
    UsbTransport transport(device, interfaceHint);
    if (transport.isOk())
        return std::make_unique<UsbTransport>(std::move(transport));

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
    auto [transfersIt, inserted] = mSendTransfers.emplace(++transferId, std::move(packet));
    if (!inserted) {
        finishSendTransfer(nullptr, mSendTransfers.end());
        return;
    }

    auto& transfers = transfersIt->second;

    auto* callbackData = new CallbackData{this,            // Has to be deleted in a callback
                                          transferId};    // (either message's or payload's)
    auto& message = transfers.packet.getMessage();
    transfers.messageTransfer = Transfer::createTransfer();

    // SUBMIT MESSAGE:
    auto messageLock = transfers.messageTransfer->getUniqueLock();
    transfers.messageTransfer->fillBulk(mHandle,
                                        mInterfaceData.writeEndpointAddress,
                                        reinterpret_cast<uint8_t*>(&message),
                                        sizeof(AMessage),
                                        staticSendMessageCallback,
                                        callbackData,
                                        0,
                                        messageLock);
    bool ok = transfers.messageTransfer->submit(messageLock);
    if (!ok) {
        std::cerr << "[UsbTransfer::send(...)] message transfer wasn't submitted, libusb_error: "
            << transfers.messageTransfer->getLastError() << std::endl;
        std::cerr << "[UsbTransfer::send(...)] packet transfer won't be completed" << std::endl;
        finishSendTransfer(callbackData, transfersIt);
        return;
    }

    messageLock.unlock();

    // SUBMIT PAYLOAD:
    if (transfers.packet.hasPayload()) {
        auto& payload = transfers.packet.getPayload();
        transfers.payloadTransfer = Transfer::createTransfer();

        auto payloadTransferLock = transfers.payloadTransfer->getUniqueLock();
        transfers.payloadTransfer->fillBulk(mHandle,
                                            mInterfaceData.writeEndpointAddress,
                                            payload.getBuffer(),
                                            payload.getSize(),
                                            staticSendPayloadCallback,
                                            callbackData,
                                            0,
                                            payloadTransferLock);

        ok = transfers.payloadTransfer->submit(payloadTransferLock);
        if (!ok) {
            std::cerr << "[UsbTransfer::send(...)] payload transfer wasn't submitted, libusb_error: "
                << transfers.payloadTransfer->getLastError() << std::endl;
            std::cerr << "[UsbTransfer::send(...)] message transfer cancelled" << std::endl;
            messageLock.lock();
            transfers.messageTransfer->cancel(messageLock);
            finishSendTransfer(callbackData, transfersIt);
        }
    }
}

void UsbTransport::receive()
{
    std::scoped_lock lock(mReceiveMutex);
    if (mIsReceiving)
        return;

    prepareToReceive(); // update buffers and transfers

    auto messageLock = mReceiveTransferPack.messageTransfer->getUniqueLock();
    mReceiveTransferPack.messageTransfer->submit(messageLock);
}

void UsbTransport::staticSendMessageCallback(const Transfer::SharedPointer& messageTransfer,
                                             const Transfer::UniqueLock& messageLock)
{
    // GET ESSENTIAL DATA:
    auto callbackData = static_cast<CallbackData*>(messageTransfer->getUserData(messageLock));
    auto* transport = callbackData->transport;
    auto transferId = callbackData->transferId;

    // FIND TRANSFERS DATA:
    std::scoped_lock sendLock(transport->mSendMutex);
    auto idPackPairIt = transport->mSendTransfers.find(transferId);
    if (idPackPairIt == transport->mSendTransfers.end()) {
        transport->finishSendTransfer(callbackData, idPackPairIt);
        return;
    } // !

    auto& transferPack = idPackPairIt->second;
    transferPack.errorCode = transferStatusToErrorCode(messageTransfer->getStatus(messageLock));

    auto& payloadTransfer = transferPack.payloadTransfer;
    if (payloadTransfer == nullptr)         // if there's no payload, we finish the transfer
        transport->finishSendTransfer(callbackData, idPackPairIt);
    else if (transferPack.errorCode != OK)  // if there's a payload and the message transfer failed
        payloadTransfer->cancel(payloadTransfer->getUniqueLock());
                                            // otherwise, we expect packet transfer to be finished in payload's callback
}

void UsbTransport::staticSendPayloadCallback(const Transfer::SharedPointer& payloadTransfer,
                                             const Transfer::UniqueLock& payloadLock)
{
    auto callbackData = static_cast<CallbackData*>(payloadTransfer->getUserData(payloadLock));
    auto* transport = callbackData->transport;
    auto transferId = callbackData->transferId;

    // FIND TRANSFERS DATA:
    std::scoped_lock sendLock(transport->mSendMutex);
    auto idPackPair = transport->mSendTransfers.find(transferId);
    if (idPackPair == transport->mSendTransfers.end()) {
        transport->finishSendTransfer(callbackData, idPackPair);
        return;
    } // !

    ErrorCode ec = transferStatusToErrorCode(payloadTransfer->getStatus(payloadLock));
    if (ec != CANCELLED || idPackPair->second.errorCode == OK)  // if this transfer wasn't cancelled or
        idPackPair->second.errorCode = ec;                      // if previous transfer is ok,
                                                                // save this transfer's error code
                                                        // (bc we don't want to override message transfer's error)

    transport->finishSendTransfer(callbackData, idPackPair);
}

void UsbTransport::staticReceiveMessageCallback(const Transfer::SharedPointer& messageTransfer,
                                                const Transfer::UniqueLock& messageLock)
{
    // GET ESSENTIAL DATA:
    auto* transport = static_cast<UsbTransport*>(messageTransfer->getUserData(messageLock));
    auto& transferPack = transport->mReceiveTransferPack;
    auto& packet = transferPack.packet;

    std::scoped_lock receiveLock(transport->mReceiveMutex);
    transferPack.errorCode = transferStatusToErrorCode(messageTransfer->getStatus(messageLock));

    if (transferPack.errorCode == OK && packet.getMessage().dataLength != 0) {
        auto payloadLock = transferPack.payloadTransfer->getUniqueLock();
        bool ok = transferPack.payloadTransfer->submit(payloadLock);
        if (!ok) {
            std::cerr << "[UsbTransport]: payload transfer was not submitted, libusb_error code: "
                << transferPack.payloadTransfer->getLastError() << std::endl;
            transferPack.errorCode = UNDERLYING_ERROR;
            transport->finishReceiveTransfer();
        }
    }
    else    // if there's error or dataLength equals zero
        transport->finishReceiveTransfer();

}

void UsbTransport::staticReceivePayloadCallback(const Transfer::SharedPointer& payloadTransfer,
                                                const Transfer::UniqueLock& payloadLock)
{
    // GET ESSENTIAL DATA:
    auto* transport = static_cast<UsbTransport*>(payloadTransfer->getUserData(payloadLock));
    auto& transferPack = transport->mReceiveTransferPack;
    auto& packet = transferPack.packet;

    std::scoped_lock receiveLock(transport->mReceiveMutex);
    transferPack.errorCode = transferStatusToErrorCode(payloadTransfer->getStatus(payloadLock));

    // Doesn't matter if there's an error or not
    packet.getPayload().setDataSize(payloadTransfer->getActualLength(payloadLock));
    transport->finishReceiveTransfer();
}

Transport::ErrorCode UsbTransport::transferStatusToErrorCode(int status) {
    if (status == LIBUSB_TRANSFER_COMPLETED)
        return ErrorCode::OK;
    else if (status == LIBUSB_TRANSFER_CANCELLED)
        return ErrorCode::CANCELLED;
    else if (status == LIBUSB_TRANSFER_NO_DEVICE)
        return ErrorCode::TRANSPORT_DISCONNECTED;
    else
        return ErrorCode::UNDERLYING_ERROR;
}

const AMessage& UsbTransport::messageFromBuffer(const uint8_t* buffer) {
    return *reinterpret_cast<const AMessage*>(buffer);
}

// This functions updates buffer for incoming packet and creates new transfers
void UsbTransport::prepareToReceive() {
    std::scoped_lock receiveLock(mReceiveMutex);

    // Update packet
    auto& packet = mReceiveTransferPack.packet;
    if (!packet.hasPayload())
        packet.movePayloadIn(APayload{mMaxPayloadSize});
    else {
        packet.getPayload().resizeBuffer(mMaxPayloadSize);
        packet.getPayload().setDataSize(0);
    }

    // Create new transfers
    auto& messageTransfer = mReceiveTransferPack.messageTransfer;
    auto& payloadTransfer = mReceiveTransferPack.payloadTransfer;
    messageTransfer = Transfer::createTransfer();
    payloadTransfer = Transfer::createTransfer();

    {   // Prepare message transfer
        auto& message = packet.getMessage();

        auto buffer = reinterpret_cast<unsigned char*>(&message);
        auto bufferSize = sizeof(AMessage);
        auto lock = messageTransfer->getUniqueLock();
        messageTransfer->fillBulk(mHandle,
                                  mInterfaceData.readEndpointAddress,
                                  buffer,
                                  bufferSize,
                                  staticReceiveMessageCallback,
                                  this,
                                  0,
                                  lock);
    }   // ! message

    {   // Prepare payload transfer
        auto& payload = packet.getPayload();

        auto buffer = payload.getBuffer();
        auto size = payload.getBufferSize();
        auto lock = payloadTransfer->getUniqueLock();
        payloadTransfer->fillBulk(mHandle,
                                  mInterfaceData.readEndpointAddress,
                                  buffer,
                                  size,
                                  staticReceivePayloadCallback,
                                  this,
                                  0,
                                  lock);
    }   // ! payload

    // Transfers are prepared but aren't submitted
}

void UsbTransport::finishSendTransfer(UsbTransport::CallbackData* callbackData,
                                      UsbTransport::TransfersContainer::iterator mapIterator)
{
    delete callbackData; // data allocated in send() method
    if (mapIterator == mSendTransfers.end())
        notifySendListener(nullptr, TRANSPORT_ERROR);
    else {
        notifySendListener(&mapIterator->second.packet, mapIterator->second.errorCode);
        mSendTransfers.erase(mapIterator);
    }
}

void UsbTransport::finishReceiveTransfer() {
    notifyReceiveListener(&mReceiveTransferPack.packet, mReceiveTransferPack.errorCode);
    mIsReceiving = false;
}
