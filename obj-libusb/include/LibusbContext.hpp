#ifndef OBJ_LIBUSB__LIBUSBCONTEXT_HPP
#define OBJ_LIBUSB__LIBUSBCONTEXT_HPP

#include <memory>
#include <vector>
#include <thread>
#include "Libusb.hpp"


class LibusbContext
        : public std::enable_shared_from_this<LibusbContext>
{
public:
    using pointer = std::shared_ptr<LibusbContext>;
    using weakPointer = pointer::weak_type;

    using LogLevel = libusb_log_level;
    //using LogCallback = void (*)(LibusbContext& context, LogLevel level, std::string message);
    using LogCallback = libusb_log_cb;
    using LogCallbackMode = libusb_log_cb_mode;

    using HotplugEvent = libusb_hotplug_event;
    //using HotplugCallback = int (*)(LibusbDevice& device, HotplugEvent event, void* userData);
    using HotplugCallback = libusb_hotplug_callback_fn;
    using HotplugCallbackHandle = libusb_hotplug_callback_handle;

public:
    static pointer makeContext();

    LibusbContext(LibusbContext&)       = delete;
    LibusbContext(const LibusbContext&) = delete;
    LibusbContext(LibusbContext&&) noexcept;
    ~LibusbContext();

    void setLogCallback(LogCallback callback, int mode); // libusb level

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
                                                  HotplugCallback callback, // libusb level
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
    LibusbContext();
    libusb_context* mContext;
};


#endif //OBJ_LIBUSB__LIBUSBCONTEXT_HPP
