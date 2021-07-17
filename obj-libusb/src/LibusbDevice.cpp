#include "LibusbDevice.hpp"
#include "LibusbError.hpp"
#include "LibusbDeviceHandle.hpp"


LibusbDevice::LibusbDevice(libusb_device *device, bool unreferenceOnDestruction)
        : mDevice(device)
        , mUnreferenceOnDestruction(unreferenceOnDestruction)
{}


LibusbDevice::LibusbDevice(LibusbDevice&& other) noexcept
    : mDevice(other.mDevice)
    , mUnreferenceOnDestruction(other.mUnreferenceOnDestruction)
{
    other.mDevice = nullptr;
    other.mUnreferenceOnDestruction = false;
}

LibusbDevice::~LibusbDevice()
{
    if (mUnreferenceOnDestruction)
        libusb_unref_device(mDevice);
}

uint8_t LibusbDevice::getBusNumber() const
{
    return libusb_get_bus_number(mDevice);
}

uint8_t LibusbDevice::getPortNumber() const
{
    return libusb_get_port_number(mDevice);
}

std::vector<uint8_t> LibusbDevice::getPortNumbers() const
{
    std::vector<uint8_t> vec(8);
    libusb_get_port_numbers(mDevice, vec.data(), vec.size());
    return vec;
}

LibusbDevice LibusbDevice::getParent() const
{
    return LibusbDevice(libusb_get_parent(mDevice));
}

uint8_t LibusbDevice::getAddress() const
{
    return libusb_get_device_address(mDevice);
}

int LibusbDevice::getSpeed() const
{
    return libusb_get_device_speed(mDevice);
}

size_t LibusbDevice::getMaxPacketSize(unsigned char endpoint) const
{
    int rc = libusb_get_max_packet_size(mDevice, endpoint);
    if (rc < 0)
        throw LibusbError(rc);

    return rc;
}

size_t LibusbDevice::getMaxIsoPacketSize(unsigned char endpoint) const
{
    int rc = libusb_get_max_iso_packet_size(mDevice, endpoint);
    if (rc < 0)
        throw LibusbError(rc);

    return rc;
}

LibusbDevice LibusbDevice::referenceDevice() const
{
    return LibusbDevice(libusb_ref_device(mDevice));
}

LibusbDeviceHandle LibusbDevice::open() const
{
    libusb_device_handle *handle{};
    CHECK_ERROR(libusb_open(mDevice, &handle));
    return LibusbDeviceHandle(handle);
}

LibusbDevice::UniqueDeviceDescriptor LibusbDevice::getDescriptor() const
{
    auto *descriptor = new DeviceDescriptor;
    CHECK_ERROR(libusb_get_device_descriptor(mDevice, descriptor));
    return LibusbDevice::UniqueDeviceDescriptor{descriptor};
}

LibusbDevice::UniqueConfigDescriptor LibusbDevice::getActiveConfigDescriptor() const
{
    ConfigDescriptor *descriptor{};
    CHECK_ERROR(libusb_get_active_config_descriptor(mDevice, &descriptor));
    return LibusbDevice::UniqueConfigDescriptor(descriptor);
}

LibusbDevice::UniqueConfigDescriptor LibusbDevice::getConfigDescriptor(uint8_t config_index) const
{
    ConfigDescriptor *descriptor{};
    CHECK_ERROR(libusb_get_config_descriptor(mDevice, config_index, &descriptor));
    return LibusbDevice::UniqueConfigDescriptor(descriptor);
}

LibusbDevice::UniqueConfigDescriptor LibusbDevice::getConfigDescriptorByValue(uint8_t bConfigurationValue) const
{
    ConfigDescriptor* descriptor{};
    CHECK_ERROR(libusb_get_config_descriptor_by_value(mDevice, bConfigurationValue, &descriptor));
    return LibusbDevice::UniqueConfigDescriptor(descriptor);
}
