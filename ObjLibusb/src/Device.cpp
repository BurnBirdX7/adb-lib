#include "ObjLibusb/Device.hpp"
#include "ObjLibusb/Error.hpp"
#include "ObjLibusb/DeviceHandle.hpp"

namespace ObjLibusb {

    Device::Device(libusb_device *device, bool unreferenceOnDestruction)
    : mDevice(device)
    , mUnreferenceOnDestruction(unreferenceOnDestruction)
    {}

    Device::Device(Device&& other) noexcept
    : mDevice(other.mDevice)
    , mUnreferenceOnDestruction(other.mUnreferenceOnDestruction)
    {
        other.mDevice = nullptr;
        other.mUnreferenceOnDestruction = false;
    }

    Device::~Device()
    {
        if (mUnreferenceOnDestruction)
            libusb_unref_device(mDevice);
    }

    uint8_t Device::getBusNumber() const
    {
        return libusb_get_bus_number(mDevice);
    }

    uint8_t Device::getPortNumber() const
    {
        return libusb_get_port_number(mDevice);
    }

    std::vector<uint8_t> Device::getPortNumbers() const
    {
        std::vector<uint8_t> vec(8);
        libusb_get_port_numbers(mDevice, vec.data(), vec.size());
        return vec;
    }

    Device Device::getParent() const
    {
        return Device(libusb_get_parent(mDevice));
    }

    uint8_t Device::getAddress() const
    {
        return libusb_get_device_address(mDevice);
    }

    int Device::getSpeed() const
    {
        return libusb_get_device_speed(mDevice);
    }

    size_t Device::getMaxPacketSize(unsigned char endpoint) const
    {
        int rc = libusb_get_max_packet_size(mDevice, endpoint);
        if (rc < 0)
            throw Error(rc);

        return rc;
    }

    size_t Device::getMaxIsoPacketSize(unsigned char endpoint) const
    {
        int rc = libusb_get_max_iso_packet_size(mDevice, endpoint);
        if (rc < 0)
            throw Error(rc);

        return rc;
    }

    Device Device::referenceDevice() const
    {
        return Device(libusb_ref_device(mDevice));
    }

    DeviceHandle Device::open() const
    {
        libusb_device_handle *handle{};
        THROW_ON_LIBUSB_ERROR(libusb_open(mDevice, &handle));
        return DeviceHandle(handle);
    }

    Descriptors::UniqueDevice Device::getDescriptor() const
    {
        auto *descriptor = new Descriptors::Device;
        THROW_ON_LIBUSB_ERROR(libusb_get_device_descriptor(mDevice, descriptor));
        return Descriptors::UniqueDevice{descriptor};
    }

    Descriptors::UniqueConfig Device::getActiveConfigDescriptor() const
    {
        Descriptors::Config* descriptor{};
        THROW_ON_LIBUSB_ERROR(libusb_get_active_config_descriptor(mDevice, &descriptor));
        return Descriptors::UniqueConfig(descriptor);
    }

    Descriptors::UniqueConfig Device::getConfigDescriptor(uint8_t config_index) const
    {
        Descriptors::Config* descriptor{};
        THROW_ON_LIBUSB_ERROR(libusb_get_config_descriptor(mDevice, config_index, &descriptor));
        return Descriptors::UniqueConfig(descriptor);
    }

    Descriptors::UniqueConfig Device::getConfigDescriptorByValue(uint8_t bConfigurationValue) const
    {
        Descriptors::Config* descriptor{};
        THROW_ON_LIBUSB_ERROR(libusb_get_config_descriptor_by_value(mDevice, bConfigurationValue, &descriptor));
        return Descriptors::UniqueConfig(descriptor);
    }

}