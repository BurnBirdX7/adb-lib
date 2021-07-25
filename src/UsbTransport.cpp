#include "UsbTransport.hpp"

#include <cassert>
#include <iostream>

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
void UsbTransport::write(const APacket& packet)
{
    auto data = new CallbackData{.packet = packet, .transport = this}; // has to be deleted in one of callbacks
    auto buffer = reinterpret_cast<unsigned char*>(&data->packet.message);
    auto bufferSize = sizeof(data->packet.message);

    auto transfer = LibusbTransfer::createTransfer();
    transfer->fillBulk(mHandle,
                       mInterfaceData.writeEndpointAddress,
                       buffer,
                       bufferSize,
                       sSendHeadCallback,
                       data,
                       0);
    transfer->submit();
}

void UsbTransport::receive()
{
    auto headBuffer = reinterpret_cast<unsigned char*>(new AMessage{});
    auto headBufferSize = sizeof(AMessage);
    auto callbackData = new CallbackData{};
    callbackData->transport = this;

    auto headTransfer = LibusbTransfer::createTransfer();
    headTransfer->fillBulk(mHandle,
                           mInterfaceData.readEndpointAddress,
                           headBuffer,
                           headBufferSize,
                           sReceiveHeadCallback,
                           callbackData,
                           0);

    headTransfer->submit();
}

void UsbTransport::sSendHeadCallback(const LibusbTransfer::Pointer& headTransfer)
{
    auto status = headTransfer->getStatus();
    if (status != LibusbTransfer::COMPLETED)
        std::cerr << "Message headTransfer has not been completed, status code: " << status << std::endl;

    auto data = static_cast<CallbackData*>(headTransfer->getUserData());
    auto& packet = data->packet;
    auto& transport = data->transport;

    // fire payload headTransfer
    if (status == LibusbTransfer::COMPLETED && packet.payload->getLength() > 0) {
        auto payloadTransfer = LibusbTransfer::createTransfer();
        auto payloadBuffer = packet.payload->getData();
        auto payloadSize = packet.payload->getLength();
        payloadTransfer->fillBulk(transport->mHandle,
                                  transport->mInterfaceData.writeEndpointAddress,
                                  payloadBuffer,
                                  payloadSize,
                                  sSendPayloadCallback,
                                  data,
                                  0);
        payloadTransfer->submit();
    }
    else {
        transport->notifySendListener(data->packet, status);
        delete data;
    }
}

void UsbTransport::sSendPayloadCallback(const LibusbTransfer::Pointer& transfer)
{
    auto status = transfer->getStatus();
    if (status != LibusbTransfer::COMPLETED)
        std::cerr << "Payload transfer has not been completed, status code: " << status << std::endl;

    auto data = static_cast<CallbackData*>(transfer->getUserData());
    auto& packet = data->packet;
    auto& transport = data->transport;

    transport->notifySendListener(data->packet, status);
    delete data;
}

void UsbTransport::sReceiveHeadCallback(const LibusbTransfer::Pointer& headTransfer)
{
    auto status = headTransfer->getStatus();
    auto data = static_cast<CallbackData*>(headTransfer->getUserData());
    auto& packet = data->packet;
    auto& transport = data->transport;

    if (status != LibusbTransfer::COMPLETED) { // if not completed, print error and skip
        std::cerr << "Payload receive headTransfer has not been completed" << std::endl;
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
        std::cerr << "Payload receive payloadTransfer has not been completed" << std::endl;
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
