#ifndef OBJLIBUSB_LIBUSBDESCRIPTORS_HPP
#define OBJLIBUSB_LIBUSBDESCRIPTORS_HPP

#include "ObjLibusb.hpp"
#include <libusb-1.0/libusb.h>


namespace ObjLibusb::Descriptors {

    // Deleter Functor
    template<class Desc, void (Func)(Desc *)>
    class Deleter
    {
    public:
        Deleter() = default;

        void operator()(Desc *desc)
        {
            Func(desc);
        }
    };

    // Unique Ptr template

    template<class Desc, void (DeleterFunc)(Desc *)>
    using Unique = std::unique_ptr<Desc, Deleter<Desc, DeleterFunc>>;

    // Descriptors:
    using Device                  = libusb_device_descriptor;
    using Endpoint                = libusb_endpoint_descriptor;
    using SsEndpointCompanion     = libusb_ss_endpoint_companion_descriptor;
    using Interface               = libusb_interface_descriptor;
    using Config                  = libusb_config_descriptor;
    using Bos                     = libusb_bos_descriptor;
    using BosDevCapability        = libusb_bos_dev_capability_descriptor;
    using Usb20Extention          = libusb_usb_2_0_extension_descriptor;
    using SsUsbDeviceCapability   = libusb_ss_usb_device_capability_descriptor;
    using ContainerId             = libusb_container_id_descriptor;


    // Unique Pointers:
    using UniqueDevice                = std::unique_ptr<Device>;
    using UniqueEndpoint              = std::unique_ptr<Endpoint>;
    using UniqueSsEndpointCompanion   = Unique<SsEndpointCompanion, libusb_free_ss_endpoint_companion_descriptor>;
    using UniqueInterface             = std::unique_ptr<Interface>;
    using UniqueConfig                = Unique<Config, libusb_free_config_descriptor>;
    using UniqueBos                   = Unique<Bos, libusb_free_bos_descriptor>;
    using UniqueBosDevCapability      = std::unique_ptr<BosDevCapability>;
    using UniqueUsb20Extention        = Unique<Usb20Extention, libusb_free_usb_2_0_extension_descriptor>;
    using UniqueSsUsbDeviceCapability = Unique<SsUsbDeviceCapability, libusb_free_ss_usb_device_capability_descriptor>;
    using UniqueContainerId           = Unique<ContainerId, libusb_free_container_id_descriptor>;

    const size_t BUFFER_SIZE = 512;
}

// Alias
namespace ObjLibusbDescriptors = ObjLibusb::Descriptors;


#endif //OBJLIBUSB_LIBUSBDESCRIPTORS_HPP
