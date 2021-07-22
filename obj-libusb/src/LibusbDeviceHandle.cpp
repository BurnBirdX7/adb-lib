#include <cassert>
#include <cstdint>
#include "LibusbDevice.hpp"
#include "LibusbDeviceHandle.hpp"
#include "LibusbError.hpp"

LibusbDeviceHandle::LibusbDeviceHandle(libusb_device_handle* handle)
    : mHandle(handle)
{
    assert(handle != nullptr);
}

LibusbDeviceHandle::LibusbDeviceHandle(LibusbDeviceHandle&& other) noexcept
    : mHandle(other.mHandle)
{
    other.mHandle = nullptr;
}

LibusbDeviceHandle::~LibusbDeviceHandle()
{
    libusb_close(mHandle);
}

LibusbDevice LibusbDeviceHandle::getDevice() const {
    return LibusbDevice(libusb_get_device(mHandle), false);
}

int LibusbDeviceHandle::getConfiguration() const {
    int config{};
    CHECK_ERROR(libusb_get_configuration(mHandle, &config))
    return config;
}

void LibusbDeviceHandle::setConfiguration(int configuration) {
    CHECK_ERROR(libusb_set_configuration(mHandle, configuration))
}

void LibusbDeviceHandle::claimInterface(int interface_number) {
    CHECK_ERROR(libusb_claim_interface(mHandle, interface_number))
}

void LibusbDeviceHandle::releaseInterface(int interface_number) {
    CHECK_ERROR(libusb_release_interface(mHandle, interface_number))
}

void LibusbDeviceHandle::setInterfaceAlternativeSetting(int interface_number, int alternate_setting) {
    CHECK_ERROR(libusb_set_interface_alt_setting(mHandle, interface_number, alternate_setting))
}

void LibusbDeviceHandle::clearHalt(unsigned char endpoint) {
    CHECK_ERROR(libusb_clear_halt(mHandle, endpoint))
}

void LibusbDeviceHandle::reset() {
    CHECK_ERROR(libusb_reset_device(mHandle))
}

bool LibusbDeviceHandle::isKernelDriverActive(int interface_number) {
    int rc = libusb_kernel_driver_active(mHandle, interface_number);
    CHECK_ERROR(rc)
    return static_cast<bool>(rc);
}

void LibusbDeviceHandle::detachKernelDriver(int interface_number) {
    CHECK_ERROR(libusb_detach_kernel_driver(mHandle, interface_number))
}

void LibusbDeviceHandle::attachKernelDriver(int interface_number) {
    CHECK_ERROR(libusb_attach_kernel_driver(mHandle, interface_number));
}

void LibusbDeviceHandle::setAutoDetachKernelDriver(bool enable) {
    CHECK_ERROR(libusb_set_auto_detach_kernel_driver(mHandle, enable));
}

LibusbDeviceHandle::UniqueBosDescriptor LibusbDeviceHandle::getBosDescriptor()
{
    BosDescriptor* descriptor{};
    CHECK_ERROR(libusb_get_bos_descriptor(mHandle, &descriptor));
    return LibusbDeviceHandle::UniqueBosDescriptor{descriptor};
}

std::vector<unsigned char> LibusbDeviceHandle::getDescriptor(LibusbDeviceHandle::DescriptorType type, uint8_t index)
{
    std::vector<unsigned char> data(DESCRIPTOR_BUFFER_SIZE);
    int rc = libusb_get_descriptor(mHandle, type, index, data.data(), data.size());
    CHECK_ERROR(rc)
    data.resize(rc);
    return data;
}

std::string LibusbDeviceHandle::getStringDescriptor(uint8_t index, uint16_t languageId)
{
    std::string str(DESCRIPTOR_BUFFER_SIZE, '\0');
    auto* data = reinterpret_cast<unsigned char*>(str.data());
    int rc = libusb_get_string_descriptor(mHandle, index, languageId, data, str.length());
    CHECK_ERROR(rc)
    str.resize(rc);
    return str;
}

std::string LibusbDeviceHandle::getStringDescriptorAscii(uint8_t index)
{
        std::string str(DESCRIPTOR_BUFFER_SIZE, '\0');
        auto* data = reinterpret_cast<unsigned char*>(str.data());
        int rc = libusb_get_string_descriptor_ascii(mHandle, index, data, str.length());
        CHECK_ERROR(rc)
        str.resize(rc);
        return str;
}

void LibusbDeviceHandle::controlTransfer(uint8_t requestType, uint8_t request, uint16_t value, uint16_t index,
                                         unsigned char *data, uint16_t length, unsigned int timeout)
{
    libusb_control_transfer(mHandle, requestType, request, value, index, data, length, timeout);
}

int LibusbDeviceHandle::bulkTransfer(uint8_t endpoint, unsigned char *data, int length, unsigned int timeout)
{
    int bytesTransferred{};
    libusb_bulk_transfer(mHandle, endpoint, data, length, &bytesTransferred, timeout);
    return bytesTransferred;
}

int LibusbDeviceHandle::interruptTransfer(uint8_t endpoint, unsigned char *data, int length, unsigned int timeout)
{
    int bytesTransferred{};
    libusb_interrupt_transfer(mHandle, endpoint, data, length, &bytesTransferred, timeout);
    return bytesTransferred;
}
