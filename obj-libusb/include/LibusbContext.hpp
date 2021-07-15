#ifndef OBJ_LIBUSB__LIBUSBCONTEXT_HPP
#define OBJ_LIBUSB__LIBUSBCONTEXT_HPP

#include <vector>
#include <thread>
#include "Libusb.hpp"


class LibusbContext {
public:
    using LogLevel = libusb_log_level;
    //using LogCallback = void (*)(LibusbContext& context, LogLevel level, std::string message);
    using LogCallback = libusb_log_cb;
    using LogCallbackMode = libusb_log_cb_mode;

    using HotplugEvent = libusb_hotplug_event;
    //using HotplugCallback = int (*)(LibusbDevice& device, HotplugEvent event, void* userData);
    using HotplugCallback = libusb_hotplug_callback_fn;
    using HotplugCallbackHandle = libusb_hotplug_callback_handle;

public:
    LibusbContext();
    LibusbContext(LibusbContext&)       = delete;
    LibusbContext(const LibusbContext&) = delete;
    LibusbContext(LibusbContext&&)      = default;
    ~LibusbContext();

    void setLogCallback(LogCallback callback, int mode);

    LibusbDeviceList getDeviceList();
    std::vector<LibusbDevice> getDeviceVector(); // Returns std::vector instead of a special class
    LibusbDeviceHandle wrapSystemDevice(intptr_t systemDevicePtr);
    LibusbDeviceHandle openDevice(uint16_t vendorId, uint16_t productId);

    // Hotplug:
    HotplugCallbackHandle registerHotplugCallback(int events,
                                                  int flags,
                                                  int vendorId,
                                                  int productId,
                                                  int deviceClass,
                                                  HotplugCallback callback,
                                                  void* userData);
    void unregisterHotplugCallback(HotplugCallbackHandle handle);

    // Polling
    std::thread spawnEventHandlingThread();

public: // USB Descriptors
    OBJLIBUSB_DESCRIPTOR_TYPES;
    UniqueSsEndpointCompanionDescriptor getSsEndpointCompanionDescriptor(EndpointDescriptor* endpointDescriptor);
    UniqueUsb20ExtentionDescriptor getUsb20ExtensionDescriptor(BosDevCapabilityDescriptor* devCapabilityDescriptor);
    UniqueSsUsbDeviceCapabilityDescriptor getSsUsbDeviceCapabilityDescriptor(BosDevCapabilityDescriptor* devCapabilityDescriptor);
    UniqueContainerIdDescriptor getContainerIdDescriptor(BosDevCapabilityDescriptor* devCapabilityDescriptor);

private:
    libusb_context* mContext;
    bool mHandleEvents;
};


#endif //OBJ_LIBUSB__LIBUSBCONTEXT_HPP
