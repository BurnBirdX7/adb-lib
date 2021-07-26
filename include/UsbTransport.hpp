#ifndef ADB_TEST_USBTRANSPORT_HPP
#define ADB_TEST_USBTRANSPORT_HPP

#include <memory>
#include <optional>
#include <map>
#include <mutex>

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
    UsbTransport(UsbTransport&&) noexcept;
    virtual ~UsbTransport();

public: // Creation
    static std::optional<UsbTransport> createTransport(const LibusbDevice& device);
    static bool isAdbInterface(const libusb_interface_descriptor& interfaceDescriptor);
    static bool isEndpointOutput(uint8_t endpointAddress);
    [[nodiscard]] bool isOk() const;

public: // Transport Interface
    void send(APacket&& packet) override;
    void receive() override;

    void setVersion(uint32_t version) override;

public: // Callbacks
    static void sSendHeadCallback(const LibusbTransfer::Pointer&);
    static void sSendPayloadCallback(const LibusbTransfer::Pointer&);

    static void sReceiveHeadCallback(const LibusbTransfer::Pointer&);
    static void sReceivePayloadCallback(const LibusbTransfer::Pointer&);

private:
    enum Flags {
        TRANSPORT_IS_OK   = 1<<0,
        INTERFACE_CLAIMED = 1<<1
    };

    struct CallbackData {
        UsbTransport* transport = {};
        size_t transferId = {};
    };

    struct Transfer {
        Transfer() = default;
        inline explicit Transfer(APacket&& packet)
            : packet(std::move(packet))
            , messageTransfer()
            , payloadTransfer()
            , errorCode(OK)
        {}

        APacket packet;
        LibusbTransfer::Pointer messageTransfer;
        LibusbTransfer::Pointer payloadTransfer;
        ErrorCode errorCode = OK;
    };

private:
    explicit UsbTransport(const LibusbDevice& device, const InterfaceData& interfaceData);
    static std::optional<InterfaceData> findAdbInterface(const LibusbDevice& device);

    static ErrorCode getTransferStatus(int);
    static const AMessage& messageFromBuffer(const uint8_t* buffer);

    void prepareReceivePacket();
    void prepareReceiveTransfers();

private:
    LibusbDevice mDevice;
    LibusbDeviceHandle mHandle;
    InterfaceData mInterfaceData;

    std::mutex mSendMutex;
    std::map<size_t /* transferId */, Transfer> mSendTransfers;
    //TODO: Replace map by more efficient container

    std::mutex mReceiveMutex;
    Transfer mReceiveTransfer;
    bool isReceiving;

    uint8_t mFlags;

};



#endif //ADB_TEST_USBTRANSPORT_HPP
