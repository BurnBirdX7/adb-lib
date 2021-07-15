#include <cassert>

#include "LibusbDeviceList.hpp"
#include "LibusbContext.hpp"
#include "LibusbDevice.hpp"
#include "LibusbError.hpp"

LibusbDeviceList::LibusbDeviceList(libusb_context *context)
        : mDeviceList(nullptr)
        , mSize(-1)
{
    ssize_t rc = libusb_get_device_list(context, &mDeviceList);
    CHECK_ERROR(rc)
    mSize = static_cast<size_t>(rc);
}

LibusbDeviceList::~LibusbDeviceList() {
    libusb_free_device_list(mDeviceList, 1);
}

LibusbDevice LibusbDeviceList::operator[](size_t index) const {
    return LibusbDevice(mDeviceList[index]);
}

LibusbDevice LibusbDeviceList::at(size_t index) const {
    assert(index < mSize);
    return LibusbDevice(mDeviceList[index], false);
}

size_t LibusbDeviceList::size() const {
    return mSize;
}