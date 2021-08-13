#include <cassert>
#include <utility>
#include "ObjLibusb/Transfer.hpp"
#include "ObjLibusb/Error.hpp"
#include "ObjLibusb/DeviceHandle.hpp"

namespace ObjLibusb {

    Transfer::Transfer(int isoPacketsNumber)
        : mTransfer(libusb_alloc_transfer(isoPacketsNumber))
        , mState(State::EMPTY)
        , mUserCallback()
        , mUserData(nullptr)
    {
        if (mTransfer == nullptr)
            throw Error(LIBUSB_ERROR_OTHER);
    }

    Transfer::~Transfer()
    {
        libusb_free_transfer(mTransfer);
    }

    Transfer::SharedPointer Transfer::createTransfer(int isoPacketsNumber)
    {
        return Transfer::SharedPointer(new Transfer(isoPacketsNumber));
    }

    void Transfer::submit(const UniqueLock& lock)
    {
        assert(isLocked(lock));
        assert(mState == READY);
        staticSubmitTransfer(*this);
    }

    void Transfer::cancel(const UniqueLock& lock)
    {
        assert(isLocked(lock));
        assert(mState == SUBMITTED);
        THROW_ON_LIBUSB_ERROR(libusb_cancel_transfer(mTransfer))
    }

    Transfer::UniqueLock Transfer::getUniqueLock()
    {
        return Transfer::UniqueLock(mMutex);
    }

    // GETTERS DEFINITION:

    uint8_t Transfer::getFlags(const UniqueLock& lock) const
    {
        assert(isLocked(lock));
        assert(mState >= READY);
        return mTransfer->flags;
    }

    uint8_t Transfer::getEndpoint(const UniqueLock& lock) const
    {
        assert(isLocked(lock));
        assert(mState >= READY);
        return mTransfer->endpoint;
    }

    uint8_t Transfer::getType(const UniqueLock& lock) const
    {
        assert(isLocked(lock));
        assert(mState >= READY);
        return mTransfer->type;
    }

    uint Transfer::getTimeout(const UniqueLock& lock) const
    {
        assert(isLocked(lock));
        assert(mState >= READY);
        return mTransfer->timeout;
    }

    uint8_t Transfer::getStatus(const UniqueLock& lock) const
    {
        assert(isLocked(lock));
        assert(mState == IN_CALLBACK);
        return mTransfer->status;
    }

    int Transfer::getLength(const UniqueLock& lock) const
    {
        assert(isLocked(lock));
        assert(mState >= READY);
        return mTransfer->length;
    }

    int Transfer::getActualLength(const UniqueLock& lock) const
    {
        assert(isLocked(lock));
        assert(mState == IN_CALLBACK);
        return mTransfer->actual_length;
    }

    uint8_t* Transfer::getBuffer(const UniqueLock& lock) const
    {
        assert(isLocked(lock));
        assert(mState >= READY);
        return mTransfer->buffer;
    }

    void* Transfer::getUserData(const UniqueLock& lock) const
    {
        assert(isLocked(lock));
        assert(mState >= READY);
        return mUserData;
    }

    void Transfer::setNewBuffer(unsigned char* buffer, uint8_t length, const UniqueLock& lock)
    {
        assert(isLocked(lock));
        assert(mState <= READY || mState == IN_CALLBACK);
        mTransfer->buffer = buffer;
        mTransfer->length = length;
    }

    void Transfer::setNewUserData(void* userData, const UniqueLock& lock)
    {
        assert(isLocked(lock));
        assert(mState <= READY || mState == IN_CALLBACK);
        mUserData = userData;
    }

    void Transfer::setNewCallback(Transfer::TransferCallback callback, const Transfer::UniqueLock& lock)
    {
        assert(isLocked(lock));
        mUserCallback = std::move(callback);
    }

    void Transfer::deleteCallback(const Transfer::UniqueLock& lock)
    {
        assert(isLocked(lock));
        mUserCallback = {};
    }


    Transfer::State Transfer::getState() const
    {
        return mState;
    }

    void Transfer::reset(const UniqueLock& lock)
    {
        assert(isLocked(lock));
        assert(mState == READY);
        const auto isoPackets = mTransfer->num_iso_packets;
        libusb_free_transfer(mTransfer);

        mTransfer = libusb_alloc_transfer(isoPackets);
        mState = EMPTY;
    }

    // FLAGS:

    void Transfer::enableFreeBufferFlag(bool enable, const UniqueLock& lock)
    {
        assert(isLocked(lock));
        assert(mState == READY);
        constexpr auto flag = Flag::FREE_BUFFER;
        if (enable)
            mTransfer->flags |= flag;
        else
            mTransfer->flags &= ~flag;
    }

    void Transfer::enableAddZeroPacketFlag(bool enable, const UniqueLock& lock)
    {
        assert(isLocked(lock));
        assert(mState == READY);
        constexpr auto flag = Flag::ADD_ZERO_PACKET;
        if (enable)
            mTransfer->flags |= flag;
        else
            mTransfer->flags &= ~flag;
    }

    // FILL TRANSFER:

    void Transfer::fillBulk(const DeviceHandle& device,
                            uint8_t endpoint,
                            unsigned char *buffer,
                            int length,
                            TransferCallback callback,
                            void *userData,
                            unsigned int timeout,
                            const UniqueLock& lock)
    {
        assert(isLocked(lock));
        assert(mState == EMPTY && "Only empty transfer can be filled, call reset() if you want to refill transfer");

        mUserCallback = std::move(callback);
        mUserData = userData;
        libusb_fill_bulk_transfer(mTransfer,
                                  device.mHandle,
                                  endpoint,
                                  buffer,
                                  length,
                                  Transfer::staticCallbackWrapper,
                                  nullptr,
                                  timeout);

        mState = READY;
    }

    void Transfer::staticCallbackWrapper(libusb_transfer* libusbTransfer)
    {
        assert(libusbTransfer && libusbTransfer->user_data);

        // Acquire shared pointer
        auto* shared = static_cast<SharedPointer*>(libusbTransfer->user_data);
        auto& transfer = *shared;

        auto lock = transfer->getUniqueLock();
        transfer->callbackWrapper(lock);

        // Destroy shared pointer
        transfer->mTransfer->user_data = nullptr;
        transfer->mState = READY;
        delete shared;
    }

    void Transfer::staticSubmitTransfer(Transfer& transfer)
    {
        // This function constructs shared pointer from *this* in dynamic memory
        // This shared pointer has to be destroyed in callback

        auto* shared = new SharedPointer{transfer.shared_from_this()};
        transfer.mTransfer->user_data = shared;
        THROW_ON_LIBUSB_ERROR(libusb_submit_transfer(transfer.mTransfer))
        transfer.mState = SUBMITTED;
    }

    void Transfer::callbackWrapper(const UniqueLock& lock)
    {
        if (mUserCallback) { // if callback is set
            mState = IN_CALLBACK;
            mUserCallback(shared_from_this(), lock);
        }
    }

    bool Transfer::isLocked(const Transfer::UniqueLock& lock) const
    {
        return (lock.mutex() == &mMutex) && lock.owns_lock();
    }

}