#ifndef ADB_LIB_USBTRANSPORT_HPP
#define ADB_LIB_USBTRANSPORT_HPP

#include <memory>
#include <optional>
#include <map>
#include <mutex>

#include <ObjLibusb-1.0/ObjLibusb.hpp>

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
    using Device = ObjLibusbDevice;
    using DeviceHandle = ObjLibusbDeviceHandle;
    using Transfer = ObjLibusbTransfer;

public:
    UsbTransport(UsbTransport&)       = delete;
    UsbTransport(const UsbTransport&) = delete;
    UsbTransport(UsbTransport&&) noexcept;
    virtual ~UsbTransport();

public: // Creation
    static std::unique_ptr<UsbTransport> make(const Device& device);
    static std::unique_ptr<UsbTransport> make(const Device& device, const InterfaceData& interfaceHint);

    static bool isAdbInterface(const ObjLibusbDescriptors::Interface& interfaceDescriptor);
    static std::optional<InterfaceData> findAdbInterface(const Device& device);

    [[nodiscard]] bool isOk() const;

public: // Transport Interface
    void send(APacket&& packet) override;
    void receive() override;

public: // Callbacks | CALLED FROM LIBUSB's EVENT HANDLING THREAD
    static void staticSendMessageCallback(const Transfer::SharedPointer&, const Transfer::UniqueLock& messageLock);
    static void staticSendPayloadCallback(const Transfer::SharedPointer&, const Transfer::UniqueLock& payloadLock);

    static void staticReceiveMessageCallback(const Transfer::SharedPointer&, const Transfer::UniqueLock& messageLock);
    static void staticReceivePayloadCallback(const Transfer::SharedPointer&, const Transfer::UniqueLock& payloadLock);

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
        // Stores a packet to transfer, message and payload transfers and error code
        TransferPack() = default;
        inline explicit TransferPack(APacket&& packet)
            : packet(std::move(packet))
            , errorCode(OK)
        {}

        APacket packet;
        Transfer::SharedPointer messageTransfer;
        Transfer::SharedPointer payloadTransfer;
        ErrorCode errorCode = OK;
    };

    using TransfersContainer = std::map<size_t /* transferPackId */, TransferPack>;

private: // Private member-functions
    explicit UsbTransport(const Device& device, const InterfaceData& interfaceData);

    // auxiliary
    static ErrorCode transferStatusToErrorCode(int libusbTransferErrorCode);
    static const AMessage& messageFromBuffer(const uint8_t* buffer);
    static bool isEndpointOutput(uint8_t endpointAddress);

    // transfers
    void prepareToReceive();
    void finishSendTransfer(CallbackData*, TransfersContainer::iterator);
    void finishReceiveTransfer();

private: // Fields
    Device mDevice;
    DeviceHandle mHandle;
    InterfaceData mInterfaceData;

    std::recursive_mutex mSendMutex;
    TransfersContainer mSendTransfers;
    // TODO: Replace map with more efficient container

    std::recursive_mutex mReceiveMutex;
    TransferPack mReceiveTransferPack;
    bool mIsReceiving = false;

    uint8_t mFlags;

};



#endif //ADB_LIB_USBTRANSPORT_HPP
