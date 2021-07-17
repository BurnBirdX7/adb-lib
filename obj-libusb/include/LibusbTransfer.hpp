#ifndef OBJ_LIBUSB_LIBUSBTRANSFER_HPP
#define OBJ_LIBUSB_LIBUSBTRANSFER_HPP

#include <memory>
#include <functional>
#include "Libusb.hpp"


// IS NOT THREAD SAFE
class LibusbTransfer
        : public std::enable_shared_from_this<LibusbTransfer>
{
public:
    using Pointer = std::shared_ptr<LibusbTransfer>;
    using WeakPointer = Pointer::weak_type;
    using TransferCallback = std::function<void(const Pointer& transfer, void* data)>;

    enum State : uint8_t {
        EMPTY           = 0,
        READY           = 1,
        SUBMITTED       = 2,
        IN_CALLBACK     = 3,
        OUT_OF_CALLBACK = 4,
    };

    enum Type : uint8_t {
        CONTROL = LIBUSB_TRANSFER_TYPE_CONTROL,
        ISOCHRONOUS = LIBUSB_TRANSFER_TYPE_ISOCHRONOUS,
        BULK = LIBUSB_TRANSFER_TYPE_BULK,
        INTERRUPT = LIBUSB_TRANSFER_TYPE_INTERRUPT,
        BULK_STREAM = LIBUSB_TRANSFER_TYPE_BULK_STREAM
    };

    enum Status : uint8_t {
        COMPLETED = LIBUSB_TRANSFER_COMPLETED,
        ERROR = LIBUSB_TRANSFER_ERROR,
        TIMED_OUT = LIBUSB_TRANSFER_TIMED_OUT,
        CANCELLED = LIBUSB_TRANSFER_CANCELLED,
        STALL = LIBUSB_TRANSFER_STALL,
        NO_DEVICE = LIBUSB_TRANSFER_NO_DEVICE,
        OVERFLOW = LIBUSB_TRANSFER_OVERFLOW
    };

    enum Flag : uint8_t {
        SHORT_NOT_OK = LIBUSB_TRANSFER_SHORT_NOT_OK,
        FREE_BUFFER = LIBUSB_TRANSFER_FREE_BUFFER,
        FREE_TRANSFER = LIBUSB_TRANSFER_FREE_TRANSFER,
        ADD_ZERO_PACKET = LIBUSB_TRANSFER_ADD_ZERO_PACKET
    };

public:
    static Pointer createTransfer(int isoPacketsNumber = 0);
    ~LibusbTransfer();

    void submit();
    void cancel();

    void enableFreeBufferFlag(bool enable = true);
    void enableAddZeroPacketFlag(bool enable = true);

    uint8_t getFlags() const;
    uint8_t getEndpoint() const;
    uint8_t getType() const;
    uint getTimeout() const;
    uint8_t getStatus() const;
    uint8_t getLength() const;
    uint8_t getActualLength() const;
    void* getUserData() const;

    State getState() const;
    void reset(); // Can be performed only in READY state

    void fillBulk(const LibusbDeviceHandle& device,
                  uint8_t                   endpoint,
                  unsigned char*            buffer,
                  int                       length,
                  TransferCallback          callback,
                  void*                     userData,
                  unsigned int              timeout);


    static void sCallbackWrapper(libusb_transfer* transfer);

private:
    struct CallbackData {
        void* userData; // the data user wants to pass to a callback function
        TransferCallback userCallback;
        Pointer transfer;
    };

    explicit LibusbTransfer(int isoPacketsNumber);
    libusb_transfer* mTransfer;
    State mState;

    CallbackData* makeCallbackData(void* userData, TransferCallback&& callback);
    void* getCallbackUserData() const;
    CallbackData* getCallbackData() const;
    void freeCallbackData();
};


#endif //OBJ_LIBUSB_LIBUSBTRANSFER_HPP
