#ifndef OBJ_LIBUSB__LIBUSBDEVICE_HPP
#define OBJ_LIBUSB__LIBUSBDEVICE_HPP

#include <vector>
#include <memory>
#include "Libusb.hpp"


class LibusbDevice {
public:
    LibusbDevice(LibusbDevice&) = delete;
    LibusbDevice(const LibusbDevice&) = delete;
    LibusbDevice(LibusbDevice&&) = default;
    ~LibusbDevice();

public:
    // Device Handling
    [[nodiscard]] uint8_t getBusNumber() const;
    [[nodiscard]] uint8_t getPortNumber() const;
    [[nodiscard]] std::vector<uint8_t> getPortNumbers() const;
    [[nodiscard]] uint8_t getAddress() const;
    [[nodiscard]] int getSpeed() const;
    [[nodiscard]] size_t getMaxPacketSize(unsigned char endpoint) const;
    [[nodiscard]] size_t getMaxIsoPacketSize(unsigned char endpoint) const;

    [[nodiscard]] LibusbDevice getParent() const;
    [[nodiscard]] LibusbDevice referenceDevice() const;
    [[nodiscard]] LibusbDeviceHandle open() const;

public: // USB Descriptors
    OBJLIBUSB_DESCRIPTOR_TYPES;
    [[nodiscard]] UniqueDeviceDescriptor getDescriptor() const;
    [[nodiscard]] UniqueConfigDescriptor getActiveConfigDescriptor() const;
    [[nodiscard]] UniqueConfigDescriptor getConfigDescriptor(uint8_t config_index) const;
    [[nodiscard]] UniqueConfigDescriptor getConfigDescriptorByValue(uint8_t bConfigurationValue) const;

protected:
    explicit LibusbDevice(libusb_device* device, bool unreferenceOnDestruction = true);
    friend LibusbContext;
    friend LibusbDeviceList;
    friend LibusbDeviceHandle;

private:
    libusb_device* mDevice;
    bool mUnreferenceOnDestruction;
};

#endif //OBJ_LIBUSB__LIBUSBDEVICE_HPP
