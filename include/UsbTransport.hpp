#ifndef ADB_TEST_USBTRANSPORT_HPP
#define ADB_TEST_USBTRANSPORT_HPP

#include <memory>
#include <optional>
#include <Libusb.hpp>
#include <LibusbDevice.hpp>
#include <LibusbDeviceHandle.hpp>

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

public:
    // CAN return empty pointer
    static std::optional<UsbTransport> makeTransport(const LibusbDevice& device);

    [[nodiscard]] bool isOk() const;

public: // Transport Interface
    void write(const APacket& packet) override;
    APacket receive() override;

private:
    explicit UsbTransport(const LibusbDevice& device, const InterfaceData& interfaceData);
    LibusbDevice mDevice;
    LibusbDeviceHandle mHandle;
    InterfaceData mInterfaceData;

    SendListener mSendListener;
    ReceiveListener mReceiveListener;

    std::vector<bool> mFlags;
    enum FlagNames {
        INTERFACE_CLAIMED = 0,
        IS_OK = 1
    };

    static std::optional<InterfaceData> findAdbInterface(const LibusbDevice& device);

};



#endif //ADB_TEST_USBTRANSPORT_HPP
