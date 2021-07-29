#include "LibusbContext.hpp"
#include "LibusbDeviceList.hpp"
#include "LibusbDeviceHandle.hpp"
#include "LibusbError.hpp"
#include "LibusbDevice.hpp"

LibusbContext::LibusbContext()
    : mContext(nullptr)
{
    libusb_init(&mContext);
}

LibusbContext::LibusbContext(LibusbContext&& other) noexcept
    : mContext(other.mContext)
{
    other.mContext = nullptr;
}

LibusbContext::~LibusbContext()
{
    libusb_exit(mContext);
}

void LibusbContext::setLogCallback(LibusbContext::LogCallback callback, int mode)
{
    libusb_set_log_cb(mContext, callback, mode);
}

LibusbDeviceList LibusbContext::getDeviceList()
{
    return LibusbDeviceList(mContext);
}

std::vector<LibusbDevice> LibusbContext::getDeviceVector()
{
    libusb_device** list{};
    ssize_t rc = libusb_get_device_list(mContext, &list);
    CHECK_LIBUSB_ERROR(rc)
    std::vector<LibusbDevice> vector{};
    vector.reserve(rc);
    for (size_t i = 0; i < rc; ++i) {
        // emplace_back is impossible because std::vector has no access to LibusbDevice's constructor
        vector.push_back(LibusbDevice{list[i], true});
    }

    libusb_free_device_list(list, 0);
    return vector;
}

LibusbDeviceHandle LibusbContext::wrapSystemDevice(intptr_t systemDevicePtr)
{
    libusb_device_handle* handle{};
    libusb_wrap_sys_device(mContext, systemDevicePtr, &handle);
    return LibusbDeviceHandle(handle);
}

LibusbDeviceHandle LibusbContext::openDevice(uint16_t vendorId, uint16_t productId)
{
    return LibusbDeviceHandle(libusb_open_device_with_vid_pid(mContext, vendorId, productId));
}

LibusbContext::HotplugCallbackHandle LibusbContext::registerHotplugCallback(int events,
                                                                            int flags,
                                                                            int vendorId,
                                                                            int productId,
                                                                            int deviceClass,
                                                                            LibusbContext::HotplugCallback callback,
                                                                            void *userData)
{
    HotplugCallbackHandle handle{};
    auto callbackWrapper = callback; // TODO: Wrapper

    int rc = libusb_hotplug_register_callback(mContext,
                                              static_cast<libusb_hotplug_event>(events),
                                              static_cast<libusb_hotplug_flag>(flags),
                                              vendorId,
                                              productId,
                                              deviceClass,
                                              callbackWrapper,
                                              userData,
                                              &handle);
    CHECK_LIBUSB_ERROR(rc)
    return handle;
}

void LibusbContext::unregisterHotplugCallback(LibusbContext::HotplugCallbackHandle handle)
{
    libusb_hotplug_deregister_callback(mContext, handle);
}

LibusbContext::UniqueSsEndpointCompanionDescriptor
LibusbContext::getSsEndpointCompanionDescriptor(LibusbContext::EndpointDescriptor *endpointDescriptor)
{
    SsEndpointCompanionDescriptor* descriptor{};
    int rc = libusb_get_ss_endpoint_companion_descriptor(mContext, endpointDescriptor, &descriptor);
    CHECK_LIBUSB_ERROR(rc)
    return LibusbContext::UniqueSsEndpointCompanionDescriptor{descriptor};
}

LibusbContext::UniqueUsb20ExtentionDescriptor
LibusbContext::getUsb20ExtensionDescriptor(LibusbContext::BosDevCapabilityDescriptor *devCapabilityDescriptor)
{
    Usb20ExtentionDescriptor* descriptor{};
    int rc = libusb_get_usb_2_0_extension_descriptor(mContext, devCapabilityDescriptor, &descriptor);
    CHECK_LIBUSB_ERROR(rc)
    return LibusbContext::UniqueUsb20ExtentionDescriptor{descriptor};
}

LibusbContext::UniqueSsUsbDeviceCapabilityDescriptor
LibusbContext::getSsUsbDeviceCapabilityDescriptor(LibusbContext::BosDevCapabilityDescriptor *devCapabilityDescriptor)
{
    SsUsbDeviceCapabilityDescriptor* descriptor{};
    int rc = libusb_get_ss_usb_device_capability_descriptor(mContext, devCapabilityDescriptor, &descriptor);
    CHECK_LIBUSB_ERROR(rc);
    return LibusbContext::UniqueSsUsbDeviceCapabilityDescriptor{descriptor};
}

LibusbContext::UniqueContainerIdDescriptor
LibusbContext::getContainerIdDescriptor(LibusbContext::BosDevCapabilityDescriptor *devCapabilityDescriptor)
{
    ContainerIdDescriptor* descriptor{};
    int rc = libusb_get_container_id_descriptor(mContext, devCapabilityDescriptor, &descriptor);
    CHECK_LIBUSB_ERROR(rc)
    return LibusbContext::UniqueContainerIdDescriptor{descriptor};
}

std::thread LibusbContext::spawnEventHandlingThread()
{
    auto weak = this->weak_from_this();
    return std::thread([weak]{
        while (!weak.expired())// while object is not expired
            libusb_handle_events(weak.lock()->mContext);
    });
}

LibusbContext::pointer LibusbContext::makeContext()
{
    return LibusbContext::pointer(new LibusbContext());
}
