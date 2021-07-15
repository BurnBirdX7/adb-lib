#ifndef OBJ_LIBUSB__LIBUSBDEVICEHANDLE_HPP
#define OBJ_LIBUSB__LIBUSBDEVICEHANDLE_HPP

#include <vector>
#include "Libusb.hpp"

class LibusbDeviceHandle {
public:
    LibusbDeviceHandle(LibusbDeviceHandle&)        = delete;
    LibusbDeviceHandle(const LibusbDeviceHandle&)  = delete;
    LibusbDeviceHandle(LibusbDeviceHandle&&)       = default;
    ~LibusbDeviceHandle();

public:
    [[nodiscard]] LibusbDevice getDevice() const;
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
    OBJLIBUSB_DESCRIPTOR_TYPES;
    using DescriptorType = libusb_descriptor_type;

    // Descriptor
    UniqueBosDescriptor getBosDescriptor();
    std::vector<unsigned char> getDescriptor(DescriptorType type, uint8_t index);
    std::string getStringDescriptor(int8_t index, uint16_t languageId);
    std::string getStringDescriptorAscii(int8_t index);

    // Synchronous I/O
    void controlTransfer(uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, unsigned char* data,
                         uint16_t length, unsigned int timeout);
    int bulkTransfer(uint8_t endpoint, unsigned char* data, int length, unsigned int timeout); // returns number of transferred bytes
    int interruptTransfer(uint8_t endpoint, unsigned char* data, int length, unsigned int timeout); // returns number of transferred bytes

protected:
    explicit LibusbDeviceHandle(libusb_device_handle*);
    friend LibusbContext;
    friend LibusbDevice;
    friend LibusbTransfer;

private:
    libusb_device_handle* mHandle;

};


#endif //OBJ_LIBUSB__LIBUSBDEVICEHANDLE_HPP
