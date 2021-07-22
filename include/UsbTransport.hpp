#ifndef ADB_TEST_USBTRANSPORT_HPP
#define ADB_TEST_USBTRANSPORT_HPP

#include <memory>
#include <optional>
#include <Libusb.hpp>
#include <LibusbDevice.hpp>
#include <LibusbDeviceHandle.hpp>
#include <LibusbTransfer.hpp>

#include "Transport.hpp"

struct InterfaceData {
    int interfaceNumber;
    uint8_t writeEndpointAddress;
    uint8_t readEndpointAddress;
    uint16_t zeroMask;
    size_t packetSize;
};

class UsbTransport
        : public Transport
{
    using SendListener = std::function<void(const APacket&, int errorCode)>;
    using ReceiveListener = std::function<void(const APacket&, int errorCode)>;

public:
    UsbTransport(UsbTransport&)       = delete;
    UsbTransport(const UsbTransport&) = delete;
    UsbTransport(UsbTransport&&) noexcept;
    virtual ~UsbTransport();

public: // Creation
    static std::optional<UsbTransport> createTransport(const LibusbDevice& device);
    static bool isAdbInterface(const libusb_interface_descriptor& interfaceDescriptor);
    static bool isEndpointOutput(uint8_t endpointAddress);
    [[nodiscard]] bool isOk() const;

public: // Transport Interface
    void write(const APacket& packet) override;
    void receive() override;

public: // Callbacks
    static void sSendHeadCallback(const LibusbTransfer::Pointer&);
    static void sSendPayloadCallback(const LibusbTransfer::Pointer&);

    static void sReceiveHeadCallback(const LibusbTransfer::Pointer&);
    static void sReceivePayloadCallback(const LibusbTransfer::Pointer&);

private:
    explicit UsbTransport(const LibusbDevice& device, const InterfaceData& interfaceData);
    LibusbDevice mDevice;
    LibusbDeviceHandle mHandle;
    InterfaceData mInterfaceData;

    uint8_t mFlags;
    enum Flags {
        TRANSPORT_IS_OK   = 1<<0,
        INTERFACE_CLAIMED = 1<<1
    };

    static std::optional<InterfaceData> findAdbInterface(const LibusbDevice& device);

    struct CallbackData {
        APacket packet{};
        UsbTransport* transport{};
    };

};



#endif //ADB_TEST_USBTRANSPORT_HPP
