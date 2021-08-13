#ifndef OBJLIBUSB_LIBUSBDEVICELIST_HPP
#define OBJLIBUSB_LIBUSBDEVICELIST_HPP

#include "ObjLibusb.hpp"

namespace ObjLibusb {

    class DeviceList {
    public:
        ~DeviceList();

        Device operator[] (size_t index) const;
        [[nodiscard]] Device at(size_t index) const;  // asserts if `index` >= size()
        [[nodiscard]] size_t size() const;

    protected:
        friend class Context;
        explicit DeviceList(libusb_context* context);

    private:
        libusb_device** mDeviceList;
        size_t mSize;
    };

}

// Alias
using ObjLibusbDeviceList = ObjLibusb::DeviceList;


#endif //OBJLIBUSB_LIBUSBDEVICELIST_HPP
