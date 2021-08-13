#include <cassert>

#include "ObjLibusb/DeviceList.hpp"
#include "ObjLibusb/Context.hpp"
#include "ObjLibusb/Device.hpp"
#include "ObjLibusb/Error.hpp"

namespace ObjLibusb {

    DeviceList::DeviceList(libusb_context *context)
    : mDeviceList(nullptr)
    , mSize(-1)
    {
        ssize_t rc = libusb_get_device_list(context, &mDeviceList);
        THROW_ON_LIBUSB_ERROR(rc)
        mSize = static_cast<size_t>(rc);
    }

    DeviceList::~DeviceList() {
        libusb_free_device_list(mDeviceList, 1);
    }

    Device DeviceList::operator[](size_t index) const {
        return Device(mDeviceList[index]);
    }

    Device DeviceList::at(size_t index) const {
        assert(index < mSize);
        return Device(mDeviceList[index], false);
    }

    size_t DeviceList::size() const {
        return mSize;
    }

}