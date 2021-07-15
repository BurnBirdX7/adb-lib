#include <cassert>
#include <iostream>
#include <memory>
#include <optional>

#include <libusb-1.0/libusb.h>

#include "LibusbContext.hpp"
#include "LibusbDevice.hpp"
#include "LibusbDeviceHandle.hpp"
#include "LibusbDeviceList.hpp"
#include "LibusbError.hpp"

#include "adb.hpp"


//void print_info(libusb_device* device);
void print_info(const LibusbDevice& device);

int main()
{
    auto* context = new LibusbContext();
    {
        auto list = context->getDeviceVector();
        if (list.empty()) {
            std::cerr << "Cannot get device list" << std::endl;
            return 1;
        }

        auto thread = context->spawnEventHandlingThread();
        thread.detach();

        std::cout << "Device count: " << list.size() << std::endl;
        for (const auto &dev : list)
            print_info(dev);
    }
    delete context;
    return 0;
}

struct InterfaceData {
    size_t interfaceNumber;
    uint8_t writeEndpointAddress;
    uint8_t readEndpointAddress;
    uint16_t zeroMask;
    size_t packetSize;
};

bool is_adb_interface(const libusb_interface_descriptor& descriptor) {
    const int usb_class = descriptor.bInterfaceClass;
    const int usb_subclass = descriptor.bInterfaceSubClass;
    const int usb_protocol = descriptor.bInterfaceProtocol;
    return (usb_class == ADB_CLASS) &&
           (usb_subclass == ADB_SUBCLASS) &&
           (usb_protocol == ADB_PROTOCOL);
}

inline bool is_endpoint_output(uint8_t endpoint) {
    return (endpoint & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT;
}

std::optional<InterfaceData> find_adb_interface(const LibusbDevice& device) {
    try {
        //auto deviceHandle = device.open();
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
    catch (LibusbError& err) {
        std::cout << "[find_adb_interface] " << err.what() << std::endl;
        return {};
    }
}

/*
void print_info(libusb_device* device) {
*/
void print_info(const LibusbDevice& device) {

    try {
        // Common
        auto descriptor = device.getDescriptor();
        std::cout
                << "Device: " << std::endl
                << "\tNumber of possible configurations: " << (int) descriptor->bNumConfigurations << std::endl
                << "\tDevice Class: " << (int) descriptor->bDeviceClass << std::endl
                << "\tVendorID: " << descriptor->idVendor << std::endl
                << "\tProductID: " << descriptor->idProduct << std::endl;

        // ADB Data
        auto optAdbInterface = find_adb_interface(device);
        if (optAdbInterface.has_value()) {
            const auto& adbInterface = optAdbInterface.value();
            std::cout << "\tADB: " << std::endl
                      << "\t\t Write Endpoint: " << static_cast<uint>(adbInterface.writeEndpointAddress) << std::endl
                      << "\t\t Read Endpoint: " << static_cast<uint>(adbInterface.readEndpointAddress) << std::endl
                      << "\t\t Packet Size: " << adbInterface.packetSize << std::endl
                      << "\t\t Interface Number: " << adbInterface.interfaceNumber << std::endl
                      << "\t\t Zero mask: " << adbInterface.zeroMask << std::endl
                      << std::endl;
        }

        std::cout << std::endl;
    }
    catch (LibusbError& err) {
        std::cerr << " [LibusbError] " << err.what() << std::endl;
    }
}
