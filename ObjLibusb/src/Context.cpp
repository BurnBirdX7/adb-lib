#include "ObjLibusb/Context.hpp"
#include "ObjLibusb/DeviceList.hpp"
#include "ObjLibusb/DeviceHandle.hpp"
#include "ObjLibusb/Error.hpp"
#include "ObjLibusb/Device.hpp"

namespace ObjLibusb {

    Context::Context()
            : mContext(nullptr) {
        libusb_init(&mContext);
    }

    Context::Context(Context &&other) noexcept
            : mContext(other.mContext) {
        other.mContext = nullptr;
    }

    Context::~Context() {
        libusb_exit(mContext);
    }

    void Context::setLogCallback(Context::LogCallback callback, int mode) {
        libusb_set_log_cb(mContext, callback, mode);
    }

    DeviceList Context::getDeviceList() {
        return DeviceList(mContext);
    }

    std::vector<Device> Context::getDeviceVector() {
        libusb_device **list{};
        ssize_t rc = libusb_get_device_list(mContext, &list);
        THROW_ON_LIBUSB_ERROR(rc)
        std::vector<Device> vector{};
        vector.reserve(rc);
        for (size_t i = 0; i < rc; ++i) {
            // emplace_back is impossible because std::vector has no access to Device's constructor
            vector.push_back(Device{list[i], true});
        }

        libusb_free_device_list(list, 0);
        return vector;
    }

    DeviceHandle Context::wrapSystemDevice(intptr_t systemDevicePtr) {
        libusb_device_handle *handle{};
        libusb_wrap_sys_device(mContext, systemDevicePtr, &handle);
        return DeviceHandle(handle);
    }

    DeviceHandle Context::openDevice(uint16_t vendorId, uint16_t productId) {
        return DeviceHandle(libusb_open_device_with_vid_pid(mContext, vendorId, productId));
    }

    Context::HotplugCallbackHandle Context::registerHotplugCallback(int events,
                                                                    int flags,
                                                                    int vendorId,
                                                                    int productId,
                                                                    int deviceClass,
                                                                    Context::HotplugCallback callback,
                                                                    void *userData) {
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
        THROW_ON_LIBUSB_ERROR(rc)
        return handle;
    }

    void Context::unregisterHotplugCallback(Context::HotplugCallbackHandle handle) {
        libusb_hotplug_deregister_callback(mContext, handle);
    }

    Descriptors::UniqueSsEndpointCompanion
    Context::getSsEndpointCompanionDescriptor(Descriptors::Endpoint* endpointDescriptor) {
        Descriptors::SsEndpointCompanion *descriptor{};
        int rc = libusb_get_ss_endpoint_companion_descriptor(mContext, endpointDescriptor, &descriptor);
        THROW_ON_LIBUSB_ERROR(rc)
        return Descriptors::UniqueSsEndpointCompanion{descriptor};
    }

    Descriptors::UniqueUsb20Extention
    Context::getUsb20ExtensionDescriptor(Descriptors::BosDevCapability* devCapabilityDescriptor) {
        Descriptors::Usb20Extention* descriptor{};
        int rc = libusb_get_usb_2_0_extension_descriptor(mContext, devCapabilityDescriptor, &descriptor);
        THROW_ON_LIBUSB_ERROR(rc)
        return Descriptors::UniqueUsb20Extention{descriptor};
    }

    Descriptors::UniqueSsUsbDeviceCapability
    Context::getSsUsbDeviceCapabilityDescriptor(Descriptors::BosDevCapability* devCapabilityDescriptor) {
        Descriptors::SsUsbDeviceCapability *descriptor{};
        int rc = libusb_get_ss_usb_device_capability_descriptor(mContext, devCapabilityDescriptor, &descriptor);
        THROW_ON_LIBUSB_ERROR(rc);
        return Descriptors::UniqueSsUsbDeviceCapability{descriptor};
    }

    Descriptors::UniqueContainerId
    Context::getContainerIdDescriptor(Descriptors::BosDevCapability* devCapabilityDescriptor) {
        Descriptors::ContainerId *descriptor{};
        int rc = libusb_get_container_id_descriptor(mContext, devCapabilityDescriptor, &descriptor);
        THROW_ON_LIBUSB_ERROR(rc)
        return Descriptors::UniqueContainerId{descriptor};
    }

    std::thread Context::spawnEventHandlingThread() {
        auto weak = this->weak_from_this();
        return std::thread([weak] {
            while (!weak.expired())// while object is not expired
                libusb_handle_events(weak.lock()->mContext);
        });
    }

    Context::pointer Context::make() {
        return Context::pointer(new Context());
    }

}