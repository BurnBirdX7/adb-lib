#ifndef OBJLIBUSB_LIBUSBDEVICE_HPP
#define OBJLIBUSB_LIBUSBDEVICE_HPP

#include <vector>
#include <memory>

#include "ObjLibusb.hpp"
#include "Descriptors.hpp"

namespace ObjLibusb {

    class Device {
    public:
        Device(Device&) = delete;
        Device(const Device&) = delete;
        Device(Device&&) noexcept ;
        ~Device();

    public:
        // Device Handling
        [[nodiscard]] uint8_t getBusNumber() const;
        [[nodiscard]] uint8_t getPortNumber() const;
        [[nodiscard]] std::vector<uint8_t> getPortNumbers() const;
        [[nodiscard]] uint8_t getAddress() const;
        [[nodiscard]] int getSpeed() const;
        [[nodiscard]] size_t getMaxPacketSize(unsigned char endpoint) const;
        [[nodiscard]] size_t getMaxIsoPacketSize(unsigned char endpoint) const;

        [[nodiscard]] Device getParent() const;
        [[nodiscard]] Device referenceDevice() const;
        [[nodiscard]] DeviceHandle open() const;

    public: // USB Descriptors

        [[nodiscard]] Descriptors::UniqueDevice getDescriptor() const;
        [[nodiscard]] Descriptors::UniqueConfig getActiveConfigDescriptor() const;
        [[nodiscard]] Descriptors::UniqueConfig getConfigDescriptor(uint8_t config_index) const;
        [[nodiscard]] Descriptors::UniqueConfig getConfigDescriptorByValue(uint8_t bConfigurationValue) const;

    protected:
        explicit Device(libusb_device* device, bool unreferenceOnDestruction = true);
        friend Context;
        friend DeviceList;
        friend DeviceHandle;

    private:
        libusb_device* mDevice;
        bool mUnreferenceOnDestruction;
    };

}

// Alias
using ObjLibusbDevice = ObjLibusb::Device;

#endif //OBJLIBUSB_LIBUSBDEVICE_HPP
