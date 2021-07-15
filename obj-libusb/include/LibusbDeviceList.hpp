#ifndef OBJ_LIBUSB__LIBUSBDEVICELIST_HPP
#define OBJ_LIBUSB__LIBUSBDEVICELIST_HPP

#include "Libusb.hpp"


class LibusbDeviceList {
public:
    ~LibusbDeviceList();

    LibusbDevice operator[] (size_t index) const;
    [[nodiscard]] LibusbDevice at(size_t index) const;  // asserts if `index` >= size()
    [[nodiscard]] size_t size() const;

protected:
    friend class LibusbContext;
    explicit LibusbDeviceList(libusb_context* context);

private:
    libusb_device** mDeviceList;
    size_t mSize;
};


#endif //OBJ_LIBUSB__LIBUSBDEVICELIST_HPP
