#ifndef ADB_LIB_USBTRANSPORT_HPP
#define ADB_LIB_USBTRANSPORT_HPP

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

public:
    UsbTransport(UsbTransport&)       = delete;
    UsbTransport(const UsbTransport&) = delete;
    UsbTransport(UsbTransport&&) noexcept;
    virtual ~UsbTransport();

public: // Creation
    static std::unique_ptr<UsbTransport> createTransport(const LibusbDevice& device);
    static std::unique_ptr<UsbTransport> createTransport(const LibusbDevice& device, const InterfaceData& interfaceHint);
    static bool isAdbInterface(const libusb_interface_descriptor& interfaceDescriptor);
    static std::optional<InterfaceData> findAdbInterface(const LibusbDevice& device);

    [[nodiscard]] bool isOk() const;

public: // Transport Interface
    void send(APacket&& packet) override;
    void receive() override;

    void setMaxPayloadSize(size_t maxPayloadSize) override;

public: // Callbacks
    static void sSendHeadCallback(const LibusbTransfer::Pointer&, const LibusbTransfer::UniqueLock& lock);
    static void sSendPayloadCallback(const LibusbTransfer::Pointer&, const LibusbTransfer::UniqueLock& lock);

    static void sReceiveHeadCallback(const LibusbTransfer::Pointer&, const LibusbTransfer::UniqueLock& lock);
    static void sReceivePayloadCallback(const LibusbTransfer::Pointer&, const LibusbTransfer::UniqueLock& lock);

private: // Definitions
    enum Flags {
        TRANSPORT_IS_OK   = 1<<0,
        INTERFACE_CLAIMED = 1<<1
    };

    struct CallbackData {
        UsbTransport* transport = {};
        size_t transferId = {};
    };

    struct TransferPack {
        TransferPack() = default;
        inline explicit TransferPack(APacket&& packet)
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

    using TransfersContainer = std::map<size_t /* transferPackId */, TransferPack>;

private: // Private member-functions
    explicit UsbTransport(const LibusbDevice& device, const InterfaceData& interfaceData);

    // auxiliary
    static ErrorCode transferStatusToErrorCode(int libusbTransferErrorCode);
    static const AMessage& messageFromBuffer(const uint8_t* buffer);
    static bool isEndpointOutput(uint8_t endpointAddress);

    // transfers
    void prepareToReceive();
    void finishSendTransfer(CallbackData*, TransfersContainer::iterator);
    void finishReceiveTransfer();

private: // Fields
    LibusbDevice mDevice;
    LibusbDeviceHandle mHandle;
    InterfaceData mInterfaceData;

    std::mutex mSendMutex;
    TransfersContainer mSendTransfers;
    //TODO: Replace map by more efficient container

    std::mutex mReceiveMutex;
    TransferPack mReceiveTransferPack;
    bool isReceiving = false;

    uint8_t mFlags;

};



#endif //ADB_LIB_USBTRANSPORT_HPP
