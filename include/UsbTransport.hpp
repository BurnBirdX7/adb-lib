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
public:
    UsbTransport(UsbTransport&)       = delete;
    UsbTransport(const UsbTransport&) = delete;
    UsbTransport(UsbTransport&&)      = default;
    virtual ~UsbTransport();

public: // Creation
    static std::optional<UsbTransport> makeTransport(const LibusbDevice& device);
    [[nodiscard]] bool isOk() const;

public: // Transport Interface
    void write(const APacket& packet) override;
    void startReceiving() override;

public: // Callbacks
    static void sSendHeadCallback(const LibusbTransfer::Pointer&, void*);
    static void sSendPayloadCallback(const LibusbTransfer::Pointer&, void*);

    static void sReceiveHeadCallback(const LibusbTransfer::Pointer&, void*);
    static void sReceivePayloadCallback(const LibusbTransfer::Pointer&, void*);

private:
    explicit UsbTransport(const LibusbDevice& device, const InterfaceData& interfaceData);
    LibusbDevice mDevice;
    LibusbDeviceHandle mHandle;
    InterfaceData mInterfaceData;

    SendListener mSendListener;
    ReceiveListener mReceiveListener;

    struct Transfers {
        using TPointer = LibusbTransfer::Pointer;
        TPointer headSend       = {};
        TPointer payloadSend    = {};
        TPointer headReceive    = {};
        TPointer payloadReceive = {};
    } mTransfers;

    uint8_t mFlags;
    enum Flags {
        TRANSPORT_IS_OK   = 1<<0,
        INTERFACE_CLAIMED = 1<<1
    };

    static std::optional<InterfaceData> findAdbInterface(const LibusbDevice& device);

};



#endif //ADB_TEST_USBTRANSPORT_HPP
