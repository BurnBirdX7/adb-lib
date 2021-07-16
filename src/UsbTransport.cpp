#include "UsbTransport.hpp"

#include <cassert>
#include <iostream>
#include <LibusbError.hpp>


void UsbTransport::write(const APacket& packet)
{
    // TODO: Write
}

APacket UsbTransport::receive()
{
    // TODO: Receive
    return APacket();
}

UsbTransport::UsbTransport(const LibusbDevice& device, const InterfaceData& interfaceData)
    : mDevice(device.reference())
    , mHandle(device.open())
    , mInterfaceData(interfaceData)
    , mFlags(2)
{
    mFlags[INTERFACE_CLAIMED] = false;
    mFlags[IS_OK] = true;

    try {
        mHandle.claimInterface(mInterfaceData.interfaceNumber);
        mFlags[INTERFACE_CLAIMED] = true;

        mHandle.clearHalt(mInterfaceData.writeEndpointAddress);
        mHandle.clearHalt(mInterfaceData.readEndpointAddress);
    }
    catch (LibusbError& err) {
        OBJLIBUSB_IOSTREAM_REPORT_ERROR(std::cerr, err);
        mFlags[IS_OK] = false;
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
    size_t interfaceNumber;

    // Run through interfaces of the device
    for (interfaceNumber = 0; interfaceNumber < interfaceCount; ++interfaceNumber) {
        const auto& interface = configDescriptor->interface[interfaceNumber];
        if (interface.num_altsetting != 1)
            continue; // skip the interface if zero altsetting OR more than one

        const auto& interfaceDescriptor = interface.altsetting[0];

        if (!is_adb_interface(interfaceDescriptor))
            continue; // skipping non-adb interface

        bool found_in = false;
        bool found_out = false;
        const size_t endpointCount = interfaceDescriptor.bNumEndpoints;

        // Run through endpoints of the interface
        for (size_t endpointNumber = 0; endpointNumber < endpointCount; ++endpointNumber) {
            const auto& endpointDescriptor = interfaceDescriptor.endpoint[endpointNumber];
            const uint8_t endpointAddress = endpointDescriptor.bEndpointAddress;
            const uint8_t endpointAttribute = endpointDescriptor.bmAttributes;
            const uint8_t transferType = endpointAttribute & LIBUSB_TRANSFER_TYPE_MASK;

            if (transferType != LIBUSB_TRANSFER_TYPE_BULK)
                continue; // skip if transfer type is not bulk

            if (is_endpoint_output(endpointAddress) && !found_out) {
                found_out = true;
                data.writeEndpointAddress = endpointAddress;
                data.zeroMask = endpointDescriptor.wMaxPacketSize - 1;
            }
            else if (!is_endpoint_output(endpointAddress) && !found_in) {
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

std::optional<UsbTransport> UsbTransport::makeTransport(const LibusbDevice& device)
{
    try {
        auto interfaceData = findAdbInterface(device);
        if (!interfaceData.has_value())
            return {}; // If interface wasn't found - return empty pointer

        auto transport = UsbTransport(device, interfaceData.value());
        if (transport.isOk())
            return transport;
    }
    catch (LibusbError& err) {
        OBJLIBUSB_IOSTREAM_REPORT_ERROR(std::cerr, err);
    }

    return {};
}

UsbTransport::~UsbTransport()
{
    if (mFlags[INTERFACE_CLAIMED])
        mHandle.releaseInterface(mInterfaceData.interfaceNumber);
}

bool UsbTransport::isOk() const
{
    return mFlags[IS_OK];
}


