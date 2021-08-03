#ifndef OBJ_LIBUSB_LIBUSBTRANSFER_HPP
#define OBJ_LIBUSB_LIBUSBTRANSFER_HPP

#include <memory>
#include <functional>
#include <variant>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include "Libusb.hpp"


// IS NOT THREAD SAFE
class LibusbTransfer
        : public std::enable_shared_from_this<LibusbTransfer>
{
public:
    using SharedLock = std::shared_lock<std::shared_mutex>;
    using UniqueLock = std::unique_lock<std::shared_mutex>;
    using VariantLock = std::variant<const SharedLock*, const UniqueLock*>;

    using Pointer = std::shared_ptr<LibusbTransfer>;
    using WeakPointer = Pointer::weak_type;
    using TransferCallback = std::function<void(const Pointer& transfer, const UniqueLock& lock)>;

    enum State : int {
        CANCELLING      = -1,
        EMPTY           =  0,
        READY           =  1,
        SUBMITTED       =  2,
        IN_CALLBACK     =  3,
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
    LibusbTransfer(const LibusbTransfer&) = delete;
    LibusbTransfer(LibusbTransfer&&) = delete;
    ~LibusbTransfer();

    static Pointer createTransfer(int isoPacketsNumber = 0);

    // Control
    void submit(const UniqueLock* lock); // READY-state only
    void cancel(const UniqueLock* lock); // SUBMITTED-state only

    // Locks
    SharedLock getSharedLock() const;
    UniqueLock getUniqueLock();

    // You can pass shared or unique lock to these functions //
    uint8_t getFlags    (const VariantLock& lock) const;
    uint8_t getEndpoint (const VariantLock& lock) const;
    uint8_t getType     (const VariantLock& lock) const;
    uint getTimeout     (const VariantLock& lock) const;
    uint8_t getStatus   (const VariantLock& lock) const;
    int getLength       (const VariantLock& lock) const;
    int getActualLength (const VariantLock& lock) const;
    uint8_t* getBuffer  (const VariantLock& lock) const;
    void* getUserData   (const VariantLock& lock) const;

    void setNewBuffer   (unsigned char* buffer, uint8_t length, const UniqueLock* lock);
    void setNewUserData (void* userData,                        const UniqueLock* lock);
    void setNewCallback (TransferCallback callback,             const UniqueLock* lock);
    void deleteCallback (                                       const UniqueLock* lock);

    // Atomic:
    State getState() const;

    // READY-state only:
    void reset                  (             const UniqueLock* lock);
    void enableFreeBufferFlag   (bool enable, const UniqueLock* lock);
    void enableAddZeroPacketFlag(bool enable, const UniqueLock* lock);

    // EMPTY-state only:
    void fillBulk(const LibusbDeviceHandle& device,
                  uint8_t                   endpoint,
                  unsigned char*            buffer,
                  int                       length,
                  TransferCallback          callback,
                  void*                     userData,
                  unsigned int              timeout,
                  const UniqueLock*         lock);

    // ...
    static void sCallbackWrapper(libusb_transfer* transfer);

private:
    // Callback's auxiliary
    void* prepareUserData();
    static WeakPointer* getWeakTransferFromUserData(void* userData);
    static void freeUserData(void* userData);

    // Threads auxiliary
    bool isVariantLocked(const VariantLock& lock) const;

    template <class L>
    bool isLocked(const L& lock) const {
        return (lock.mutex() == &mMutex) && lock.owns_lock();
    }

private:
    explicit LibusbTransfer(int isoPacketsNumber);

    mutable std::shared_mutex mMutex;

    libusb_transfer* mTransfer;
    std::atomic<State> mState;
    TransferCallback mUserCallback;
    void* mUserData;
};


#endif //OBJ_LIBUSB_LIBUSBTRANSFER_HPP
