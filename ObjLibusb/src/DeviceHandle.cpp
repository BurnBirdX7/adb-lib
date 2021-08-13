#include <cassert>
#include <cstdint>
#include "ObjLibusb/Device.hpp"
#include "ObjLibusb/DeviceHandle.hpp"
#include "ObjLibusb/Error.hpp"

namespace ObjLibusb {

    DeviceHandle::DeviceHandle(libusb_device_handle* handle)
            : mHandle(handle)
    {
        assert(handle != nullptr);
    }

    DeviceHandle::DeviceHandle(DeviceHandle&& other) noexcept
            : mHandle(other.mHandle)
    {
        other.mHandle = nullptr;
    }

    DeviceHandle::~DeviceHandle()
    {
        libusb_close(mHandle);
    }

    Device DeviceHandle::getDevice() const
    {
        return Device(libusb_get_device(mHandle), false);
    }

    int DeviceHandle::getConfiguration() const
    {
        int config{};
        THROW_ON_LIBUSB_ERROR(libusb_get_configuration(mHandle, &config))
        return config;
    }

    void DeviceHandle::setConfiguration(int configuration)
    {
        THROW_ON_LIBUSB_ERROR(libusb_set_configuration(mHandle, configuration))
    }

    void DeviceHandle::claimInterface(int interface_number)
    {
        THROW_ON_LIBUSB_ERROR(libusb_claim_interface(mHandle, interface_number))
    }

    void DeviceHandle::releaseInterface(int interface_number)
    {
        THROW_ON_LIBUSB_ERROR(libusb_release_interface(mHandle, interface_number))
    }

    void DeviceHandle::setInterfaceAlternativeSetting(int interface_number, int alternate_setting)
    {
        THROW_ON_LIBUSB_ERROR(libusb_set_interface_alt_setting(mHandle, interface_number, alternate_setting))
    }

    void DeviceHandle::clearHalt(unsigned char endpoint)
    {
        THROW_ON_LIBUSB_ERROR(libusb_clear_halt(mHandle, endpoint))
    }

    void DeviceHandle::reset()
    {
        THROW_ON_LIBUSB_ERROR(libusb_reset_device(mHandle))
    }

    bool DeviceHandle::isKernelDriverActive(int interface_number)
    {
        int rc = libusb_kernel_driver_active(mHandle, interface_number);
        THROW_ON_LIBUSB_ERROR(rc)
        return static_cast<bool>(rc);
    }

    void DeviceHandle::detachKernelDriver(int interface_number)
    {
        THROW_ON_LIBUSB_ERROR(libusb_detach_kernel_driver(mHandle, interface_number))
    }

    void DeviceHandle::attachKernelDriver(int interface_number)
    {
        THROW_ON_LIBUSB_ERROR(libusb_attach_kernel_driver(mHandle, interface_number));
    }

    void DeviceHandle::setAutoDetachKernelDriver(bool enable)
    {
        THROW_ON_LIBUSB_ERROR(libusb_set_auto_detach_kernel_driver(mHandle, enable));
    }

    Descriptors::UniqueBos DeviceHandle::getBosDescriptor()
    {
        Descriptors::Bos* descriptor{};
        THROW_ON_LIBUSB_ERROR(libusb_get_bos_descriptor(mHandle, &descriptor));
        return Descriptors::UniqueBos{descriptor};
    }

    std::vector<unsigned char> DeviceHandle::getDescriptor(DeviceHandle::DescriptorType type, uint8_t index)
    {
        std::vector<unsigned char> data(Descriptors::BUFFER_SIZE);
        int rc = libusb_get_descriptor(mHandle, type, index, data.data(), data.size());
        THROW_ON_LIBUSB_ERROR(rc)
        data.resize(rc);
        return data;
    }

    std::string DeviceHandle::getStringDescriptor(uint8_t index, uint16_t languageId)
    {
        std::string str(Descriptors::BUFFER_SIZE, '\0');
        auto* data = reinterpret_cast<unsigned char*>(str.data());
        int rc = libusb_get_string_descriptor(mHandle, index, languageId, data, str.length());
        THROW_ON_LIBUSB_ERROR(rc)
        str.resize(rc);
        return str;
    }

    std::string DeviceHandle::getStringDescriptorAscii(uint8_t index)
    {
        std::string str(Descriptors::BUFFER_SIZE, '\0');
        auto* data = reinterpret_cast<unsigned char*>(str.data());
        int rc = libusb_get_string_descriptor_ascii(mHandle, index, data, str.length());
        THROW_ON_LIBUSB_ERROR(rc)
        str.resize(rc);
        return str;
    }

    void DeviceHandle::controlTransfer(uint8_t requestType, uint8_t request, uint16_t value, uint16_t index,
                                             unsigned char* data, uint16_t length, unsigned int timeout)
    {
        libusb_control_transfer(mHandle, requestType, request, value, index, data, length, timeout);
    }

    int DeviceHandle::bulkTransfer(uint8_t endpoint, unsigned char* data, int length, unsigned int timeout)
    {
        int bytesTransferred{};
        libusb_bulk_transfer(mHandle, endpoint, data, length, &bytesTransferred, timeout);
        return bytesTransferred;
    }

    int DeviceHandle::interruptTransfer(uint8_t endpoint, unsigned char* data, int length, unsigned int timeout)
    {
        int bytesTransferred{};
        libusb_interrupt_transfer(mHandle, endpoint, data, length, &bytesTransferred, timeout);
        return bytesTransferred;
    }

}