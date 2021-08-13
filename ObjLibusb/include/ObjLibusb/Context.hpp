#ifndef OBJLIBUSB_LIBUSBCONTEXT_HPP
#define OBJLIBUSB_LIBUSBCONTEXT_HPP

#include <memory>
#include <vector>
#include <thread>

#include "ObjLibusb.hpp"
#include "Descriptors.hpp"


namespace ObjLibusb {

    class Context
        : public std::enable_shared_from_this<Context>
    {
    public:
        using pointer = std::shared_ptr<Context>;
        using weakPointer = pointer::weak_type;

        using LogLevel = libusb_log_level;
        //using LogCallback = void (*)(Context& context, LogLevel level, std::string message);
        using LogCallback = libusb_log_cb;
        using LogCallbackMode = libusb_log_cb_mode;

        using HotplugEvent = libusb_hotplug_event;
        //using HotplugCallback = int (*)(Device& device, HotplugEvent event, void* userData);
        using HotplugCallback = libusb_hotplug_callback_fn;
        using HotplugCallbackHandle = libusb_hotplug_callback_handle;

    public:
        static pointer make();

        Context(Context&)       = delete;
        Context(const Context&) = delete;
        Context(Context&&) noexcept;
        ~Context();

        void setLogCallback(LogCallback callback, int mode); // libusb level

        DeviceList getDeviceList();
        std::vector<Device> getDeviceVector(); // Returns std::vector instead of a special class
        DeviceHandle wrapSystemDevice(intptr_t systemDevicePtr);
        DeviceHandle openDevice(uint16_t vendorId, uint16_t productId);

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

        Descriptors::UniqueSsEndpointCompanion getSsEndpointCompanionDescriptor(Descriptors::Endpoint* endpointDescriptor);
        Descriptors::UniqueUsb20Extention getUsb20ExtensionDescriptor(Descriptors::BosDevCapability* devCapabilityDescriptor);
        Descriptors::UniqueSsUsbDeviceCapability getSsUsbDeviceCapabilityDescriptor(Descriptors::BosDevCapability* devCapabilityDescriptor);
        Descriptors::UniqueContainerId getContainerIdDescriptor(Descriptors::BosDevCapability* devCapabilityDescriptor);

    private:
        Context();
        libusb_context* mContext;
    };

}

// Alias
using ObjLibusbContext = ObjLibusb::Context;


#endif //OBJLIBUSB_LIBUSBCONTEXT_HPP
