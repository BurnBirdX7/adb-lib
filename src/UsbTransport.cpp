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

    auto* callbackData = new CallbackData{.transport = this, .transferId = transferId};
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
    if (mReceiveTransfer.has_value())
        return;

    auto headBuffer = reinterpret_cast<unsigned char*>(&mReceivePacket.getMessage());
    auto headBufferSize = sizeof(AMessage);

    auto headTransfer = LibusbTransfer::createTransfer();
    headTransfer->fillBulk(mHandle,
                           mInterfaceData.readEndpointAddress,
                           headBuffer,
                           headBufferSize,
                           sReceiveHeadCallback,
                           this,
                           0);

    headTransfer->submit();
}

void UsbTransport::sSendHeadCallback(const LibusbTransfer::Pointer& headTransfer)
{
    // GET ESSENTIAL DATA:
    std::unique_ptr<CallbackData> callbackData{static_cast<CallbackData*>(headTransfer->getUserData())};
    auto* transport = callbackData->transport;
    auto transferId = callbackData->transferId;

    // FIND TRANSFERS DATA:
    std::scoped_lock lock(transport->mSendMutex);
    auto transfers = transport->mSendTransfers.find(transferId);
    if (transfers == transport->mSendTransfers.end()) {
        transport->notifySendListener(nullptr, TRANSPORT_ERROR);
        return;
    } // !

    ErrorCode ec = getTransferStatus(headTransfer->getStatus());
    // OK:
    if (ec == ErrorCode::OK && transfers->second.payloadTransfer == nullptr) {  // if there's a payload transfer
        transport->notifySendListener(&transfers->second.packet, ec);           // we expect notification to happen
                                                                                // in payload's callback

        callbackData.release();  // Release callback data pointer to make it available in payload's callback
        return;
    }

    // NOT OK:
    transfers->second.errorCode = ec;
    if (transfers->second.payloadTransfer != nullptr)
        transfers->second.payloadTransfer->cancel();
    else
        transport->notifySendListener(&transfers->second.packet, ec);
}

void UsbTransport::sSendPayloadCallback(const LibusbTransfer::Pointer& payloadTransfer)
{
    std::unique_ptr<CallbackData> callbackData{static_cast<CallbackData*>(payloadTransfer->getUserData())};
    auto* transport = callbackData->transport;
    auto transferId = callbackData->transferId;

    // FIND TRANSFERS DATA:
    std::scoped_lock lock(transport->mSendMutex);
    auto transfers = transport->mSendTransfers.find(transferId);
    if (transfers == transport->mSendTransfers.end()) {
        transport->notifySendListener(nullptr, TRANSPORT_ERROR);
        return;
    } // !

    ErrorCode ec = getTransferStatus(payloadTransfer->getStatus());
    if (ec == CANCELLED && transfers->second.errorCode != OK)   // if this transfer was cancelled and previous transfer
        ec = transfers->second.errorCode;                       // is not ok, report first's transfer error

    transport->notifySendListener(&transfers->second.packet, ec);
}

void UsbTransport::sReceiveHeadCallback(const LibusbTransfer::Pointer& headTransfer)
{
    auto status = headTransfer->getStatus();
    auto data = static_cast<CallbackData*>(headTransfer->getUserData());
    auto& packet = data->packet;
    auto& transport = data->transport;

    if (status != LibusbTransfer::COMPLETED) { // if not completed, print error and skip
        std::cerr << "APayload receive headTransfer has not been completed" << std::endl;
    }
    else { // if completed
        auto* message = reinterpret_cast<AMessage*>(headTransfer->getBuffer()); // interpret pointer to buffer
                                                                            // as pointer to AMessage
        packet.message = *message;  // copy incoming message to the packet

        // if dataLength is greater than 0 then try to read payload
        if (packet.message.dataLength > 0) {
            auto payloadTransfer = LibusbTransfer::createTransfer();
            // TODO: Safe memory allocation
            auto payloadBuffer = static_cast<unsigned char*>(std::malloc(MAX_PAYLOAD));
            auto payloadSize = MAX_PAYLOAD;
            payloadTransfer->fillBulk(transport->mHandle,
                                      transport->mInterfaceData.readEndpointAddress,
                                      payloadBuffer,
                                      payloadSize,
                                      sReceivePayloadCallback,
                                      data,
                                      0);

            payloadTransfer->submit();
            return; // if we submit new transport we should not free data
        }
    }

    transport->notifyReceiveListener(packet, status);
    delete data;
}

void UsbTransport::sReceivePayloadCallback(const LibusbTransfer::Pointer& payloadTransfer)
{
    auto status = payloadTransfer->getStatus();
    auto data = static_cast<CallbackData*>(payloadTransfer->getUserData());
    auto& transport = data->transport;
    auto& packet = data->packet;

    if (status != LibusbTransfer::COMPLETED) {
        std::cerr << "APayload receive payloadTransfer has not been completed" << std::endl;
    }
    else {
        auto buffer = payloadTransfer->getBuffer();
        auto length = payloadTransfer->getLength();
        auto actualLength = payloadTransfer->getActualLength();
        auto payload = new SimplePayload(buffer, length);  // SimplePayload is now responsible for the memory
        payload->resize(actualLength);              // Shrink
        packet.payload = payload;                   // APacket is now responsible for the payload
        packet.deletePayloadOnDestruction = true;
    }
    transport->notifyReceiveListener(packet, status);
    delete data;
}

Transport::ErrorCode UsbTransport::getTransferStatus(int status) {
    if (status == LIBUSB_TRANSFER_COMPLETED)
        return ErrorCode::OK;
    else if (status == LIBUSB_TRANSFER_CANCELLED)
        return ErrorCode::CANCELLED;
    else
        return ErrorCode::UNDERLYING_ERROR;
}
