#ifndef OBJ_LIBUSB__LIBUSBDESCRIPTORS_HPP
#define OBJ_LIBUSB__LIBUSBDESCRIPTORS_HPP


// USB Descriptors Types


#define OBJLIBUSB_DESCRIPTOR_TYPES                                                                                  \
using DeviceDescriptor                  = libusb_device_descriptor;                                                 \
using EndpointDescriptor                = libusb_endpoint_descriptor;                                               \
using SsEndpointCompanionDescriptor     = libusb_ss_endpoint_companion_descriptor;                                  \
using InterfaceDescriptor               = libusb_interface_descriptor;                                              \
using ConfigDescriptor                  = libusb_config_descriptor;                                                 \
using BosDescriptor                     = libusb_bos_descriptor;                                                    \
using BosDevCapabilityDescriptor        = libusb_bos_dev_capability_descriptor;                                     \
using Usb20ExtentionDescriptor          = libusb_usb_2_0_extension_descriptor;                                      \
using SsUsbDeviceCapabilityDescriptor   = libusb_ss_usb_device_capability_descriptor;                               \
using ContainerIdDescriptor             = libusb_container_id_descriptor;                                           \
                                                                                                                    \
                                                                                                                    \
using UniqueDeviceDescriptor                = std::unique_ptr<DeviceDescriptor>;                                    \
using UniqueEndpointDescriptor              = std::unique_ptr<EndpointDescriptor>;                                  \
using UniqueSsEndpointCompanionDescriptor   = Unique<SsEndpointCompanionDescriptor,                                 \
                                                     libusb_free_ss_endpoint_companion_descriptor>;                 \
using UniqueInterfaceDescriptor             = std::unique_ptr<InterfaceDescriptor>;                                 \
using UniqueConfigDescriptor                = Unique<ConfigDescriptor, libusb_free_config_descriptor>;              \
using UniqueBosDescriptor                   = Unique<BosDescriptor, libusb_free_bos_descriptor>;                    \
using UniqueBosDevCapabilityDescriptor      = std::unique_ptr<BosDevCapabilityDescriptor>;                          \
using UniqueUsb20ExtentionDescriptor        = Unique<Usb20ExtentionDescriptor,                                      \
                                                     libusb_free_usb_2_0_extension_descriptor>;                     \
using UniqueSsUsbDeviceCapabilityDescriptor = Unique<SsUsbDeviceCapabilityDescriptor,                               \
                                                     libusb_free_ss_usb_device_capability_descriptor>;              \
using UniqueContainerIdDescriptor           = Unique<ContainerIdDescriptor, libusb_free_container_id_descriptor>;


// TODO: Namespace
/*
#define OBJLIBUSB_DESCRIPTOR_TYPES                       \
using ObjLibusb::DeviceDescriptor;                       \
using ObjLibusb::EndpointDescriptor;                     \
using ObjLibusb::SsEndpointCompanionDescriptor;          \
using ObjLibusb::InterfaceDescriptor;                    \
using ObjLibusb::ConfigDescriptor;                       \
using ObjLibusb::BosDescriptor;                          \
using ObjLibusb::BosDevCapabilityDescriptor;             \
using ObjLibusb::Usb20ExtentionDescriptor;               \
using ObjLibusb::SsUsbDeviceCapabilityDescriptor;        \
using ObjLibusb::ContainerIdDescriptor;                  \
                                                         \
using ObjLibusb::UniqueDeviceDescriptor;                 \
using ObjLibusb::UniqueEndpointDescriptor;               \
using ObjLibusb::UniqueSsEndpointCompanionDescriptor;    \
using ObjLibusb::UniqueInterfaceDescriptor;              \
using ObjLibusb::UniqueConfigDescriptor;                 \
using ObjLibusb::UniqueBosDescriptor;                    \
using ObjLibusb::UniqueBosDevCapabilityDescriptor;       \
using ObjLibusb::UniqueUsb20ExtentionDescriptor;         \
using ObjLibusb::UniqueSsUsbDeviceCapabilityDescriptor;  \
using ObjLibusb::UniqueContainerIdDescriptor;


namespace ObjLibusb {
    template<class Desc, void (Func)(Desc *)>
    class Deleter
    {
    public:
        Deleter() = default;
        void operator()(Desc *desc) {
            Func(desc);
        }
    };

    template<class Desc, void (DeleterFunc)(Desc *)>
    using Unique = std::unique_ptr<Desc, Deleter<Desc, DeleterFunc>>;

    const size_t DESCRIPTOR_BUFFER_SIZE = 512;

    using DeviceDescriptor = libusb_device_descriptor;
    using EndpointDescriptor = libusb_endpoint_descriptor;
    using SsEndpointCompanionDescriptor = libusb_ss_endpoint_companion_descriptor;
    using InterfaceDescriptor = libusb_interface_descriptor;
    using ConfigDescriptor = libusb_config_descriptor;
    using BosDescriptor = libusb_bos_descriptor;
    using BosDevCapabilityDescriptor = libusb_bos_dev_capability_descriptor;
    using Usb20ExtentionDescriptor = libusb_usb_2_0_extension_descriptor;
    using SsUsbDeviceCapabilityDescriptor = libusb_ss_usb_device_capability_descriptor;
    using ContainerIdDescriptor = libusb_container_id_descriptor;

    using UniqueDeviceDescriptor = std::unique_ptr<DeviceDescriptor>;
    using UniqueEndpointDescriptor = std::unique_ptr<EndpointDescriptor>;
    using UniqueSsEndpointCompanionDescriptor = Unique<SsEndpointCompanionDescriptor,
                                                       libusb_free_ss_endpoint_companion_descriptor>;
    using UniqueInterfaceDescriptor = std::unique_ptr<InterfaceDescriptor>;
    using UniqueConfigDescriptor = Unique<ConfigDescriptor, libusb_free_config_descriptor>;
    using UniqueBosDescriptor = Unique<BosDescriptor, libusb_free_bos_descriptor>;
    using UniqueBosDevCapabilityDescriptor = std::unique_ptr<BosDevCapabilityDescriptor>;
    using UniqueUsb20ExtentionDescriptor = Unique<Usb20ExtentionDescriptor,
                                                  libusb_free_usb_2_0_extension_descriptor>;
    using UniqueSsUsbDeviceCapabilityDescriptor = Unique<SsUsbDeviceCapabilityDescriptor,
                                                         libusb_free_ss_usb_device_capability_descriptor>;
    using UniqueContainerIdDescriptor = Unique<ContainerIdDescriptor, libusb_free_container_id_descriptor>;
}
*/

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

template<class Desc, void (DeleterFunc)(Desc *)>
using Unique = std::unique_ptr<Desc, Deleter<Desc, DeleterFunc>>;


const size_t DESCRIPTOR_BUFFER_SIZE = 512;


#endif //OBJ_LIBUSB__LIBUSBDESCRIPTORS_HPP
