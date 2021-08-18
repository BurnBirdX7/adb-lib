#ifndef OBJLIBUSB_LIBUSBDEVICEHANDLE_HPP
#define OBJLIBUSB_LIBUSBDEVICEHANDLE_HPP

#include <vector>
#include <cstdint>
#include <string>

#include "ObjLibusb.hpp"
#include "Descriptors.hpp"

namespace ObjLibusb {

    class DeviceHandle {
    public:
        DeviceHandle(DeviceHandle&)        = delete;
        DeviceHandle(const DeviceHandle&)  = delete;
        DeviceHandle(DeviceHandle&&) noexcept;
        ~DeviceHandle();

    public:
        [[nodiscard]] Device getDevice() const;
        [[nodiscard]] int getConfiguration() const;
        void setConfiguration(int configuration);
        void claimInterface(int interface_number);
        void releaseInterface(int interface_number);
        void setInterfaceAlternativeSetting(int interface_number, int alternate_setting);
        void clearHalt(unsigned char endpoint);
        void reset();

        // Kernel
        bool isKernelDriverActive(int interface_number);
        void detachKernelDriver(int interface_number);   // Darwin and Windows are unsupported
        void attachKernelDriver(int interface_number);   // Linux-only
        void setAutoDetachKernelDriver(bool enable);

    public: // USB Descriptors
        using DescriptorType = libusb_descriptor_type;

        // Descriptor
        Descriptors::UniqueBos getBosDescriptor();
        std::vector<unsigned char> getDescriptor(DescriptorType type, uint8_t index);
        std::string getStringDescriptor(uint8_t index, uint16_t languageId);
        std::string getStringDescriptorAscii(uint8_t index);

        // Synchronous I/O
        void controlTransfer(uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, unsigned char* data,
                             uint16_t length, unsigned int timeout);
        int bulkTransfer(uint8_t endpoint, unsigned char* data, int length, unsigned int timeout); // returns number of transferred bytes
        int interruptTransfer(uint8_t endpoint, unsigned char* data, int length, unsigned int timeout); // returns number of transferred bytes

    protected:
        explicit DeviceHandle(libusb_device_handle*);
        friend Context;
        friend Device;
        friend Transfer;

    private:
        libusb_device_handle* mHandle;

    };

}

// Alias
using ObjLibusbDeviceHandle = ObjLibusb::DeviceHandle;


#endif //OBJLIBUSB_LIBUSBDEVICEHANDLE_HPP
