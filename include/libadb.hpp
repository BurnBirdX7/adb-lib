#ifndef USB_TEST_ADB_HPP
#define USB_TEST_ADB_HPP

#include <optional>
#include <Libusb.hpp>

#include "adb.hpp"

// ADB Utilities
struct interface_data {
    size_t interfaceNumber;
    uint8_t writeEndpointAddress;
    uint8_t readEndpointAddress;
    uint16_t zeroMask;
    size_t packetSize;
};

bool is_adb_interface(const libusb_interface_descriptor& descriptor);
inline bool is_endpoint_output(uint8_t endpoint);
std::optional<interface_data> find_adb_interface(const LibusbDevice& device);



#endif //USB_TEST_ADB_HPP
