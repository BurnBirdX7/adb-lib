#include <cassert>
#include <utility>
#include "LibusbTransfer.hpp"
#include "LibusbError.hpp"
#include "LibusbDeviceHandle.hpp"

LibusbTransfer::LibusbTransfer(int isoPacketsNumber)
    : mTransfer(libusb_alloc_transfer(isoPacketsNumber))
    , mState(State::EMPTY)
    , mUserCallback()
    , mUserData(nullptr)
{
    if (mTransfer == nullptr)
        throw LibusbError(LIBUSB_ERROR_OTHER);
}

LibusbTransfer::~LibusbTransfer()
{
    libusb_free_transfer(mTransfer);
}

LibusbTransfer::Pointer LibusbTransfer::createTransfer(int isoPacketsNumber)
{
    return LibusbTransfer::Pointer(new LibusbTransfer(isoPacketsNumber));
}

void LibusbTransfer::submit()
{
    UniqueLock lock(mMutex);
    assert(mState == READY);
    mTransfer->user_data = prepareUserData();
    CHECK_LIBUSB_ERROR(libusb_submit_transfer(mTransfer))
    mState = SUBMITTED;
}

void LibusbTransfer::cancel()
{
    UniqueLock lock(mMutex);
    assert(mState == SUBMITTED);
    CHECK_LIBUSB_ERROR(libusb_cancel_transfer(mTransfer))
}

LibusbTransfer::SharedLock LibusbTransfer::getSharedLock() const
{
    return LibusbTransfer::SharedLock(mMutex);
}

LibusbTransfer::UniqueLock LibusbTransfer::getUniqueLock()
{
    return LibusbTransfer::UniqueLock(mMutex);
}

// GETTERS DEFINITION:

uint8_t LibusbTransfer::getFlags(const VariantLock& lock) const
{
    auto optLock = processVariantLock(lock);
    assert(mState >= READY);
    return mTransfer->flags;
}

uint8_t LibusbTransfer::getEndpoint(const VariantLock& lock) const
{
    auto optLock = processVariantLock(lock);
    assert(mState >= READY);
    return mTransfer->endpoint;
}

uint8_t LibusbTransfer::getType(const VariantLock& lock) const
{
    auto optLock = processVariantLock(lock);
    assert(mState >= READY);
    return mTransfer->type;
}

uint LibusbTransfer::getTimeout(const VariantLock& lock) const
{
    auto optLock = processVariantLock(lock);
    assert(mState >= READY);
    return mTransfer->timeout;
}

uint8_t LibusbTransfer::getStatus(const VariantLock& lock) const
{
    auto optLock = processVariantLock(lock);
    assert(mState == IN_CALLBACK);
    return mTransfer->status;
}

int LibusbTransfer::getLength(const VariantLock& lock) const
{
    auto optLock = processVariantLock(lock);
    assert(mState >= READY);
    return mTransfer->length;
}

int LibusbTransfer::getActualLength(const VariantLock& lock) const
{
    auto optLock = processVariantLock(lock);
    assert(mState == IN_CALLBACK);
    return mTransfer->actual_length;
}

uint8_t* LibusbTransfer::getBuffer(const VariantLock& lock) const
{
    auto optLock = processVariantLock(lock);
    assert(mState >= READY);
    return mTransfer->buffer;
}

void* LibusbTransfer::getUserData(const VariantLock& lock) const
{
    auto optLock = processVariantLock(lock);
    assert(mState >= READY);
    return mUserData;
}

void LibusbTransfer::setNewBuffer(unsigned char* buffer, uint8_t length, const UniqueLock* lock)
{
    auto optLock = processUniqueLock(lock);
    assert(mState <= READY || mState == IN_CALLBACK);
    mTransfer->buffer = buffer;
    mTransfer->length = length;
}

void LibusbTransfer::setNewUserData(void* userData, const UniqueLock* lock)
{
    auto optLock = processUniqueLock(lock);
    assert(mState <= READY || mState == IN_CALLBACK);
    mUserData = userData;
}

void LibusbTransfer::setNewCallback(LibusbTransfer::TransferCallback callback, const LibusbTransfer::UniqueLock* lock)
{
    auto optLock = processUniqueLock(lock);
    mUserCallback = std::move(callback);
}

void LibusbTransfer::deleteCallback(const LibusbTransfer::UniqueLock* lock)
{
    auto optLock = processUniqueLock(lock);
    mUserCallback = {};
}


LibusbTransfer::State LibusbTransfer::getState() const
{
    return mState;
}

void LibusbTransfer::reset(const UniqueLock* lock)
{
    auto optLock = processUniqueLock(lock);
    assert(mState == READY);
    const auto isoPackets = mTransfer->num_iso_packets;
    libusb_free_transfer(mTransfer);

    mTransfer = libusb_alloc_transfer(isoPackets);
    mState = EMPTY;
}

void LibusbTransfer::enableFreeBufferFlag(bool enable, const UniqueLock* lock)
{
    auto optLock = processUniqueLock(lock);
    assert(mState == READY);
    constexpr auto flag = Flag::FREE_BUFFER;
    if (enable)
        mTransfer->flags |= flag;
    else
        mTransfer->flags &= ~flag;
}

void LibusbTransfer::enableAddZeroPacketFlag(bool enable, const UniqueLock* lock)
{
    auto optLock = processUniqueLock(lock);
    assert(mState == READY);
    constexpr auto flag = Flag::ADD_ZERO_PACKET;
    if (enable)
        mTransfer->flags |= flag;
    else
        mTransfer->flags &= ~flag;
}

void LibusbTransfer::fillBulk(const LibusbDeviceHandle &device, uint8_t endpoint, unsigned char *buffer, int length,
                              TransferCallback callback, void *userData, unsigned int timeout, const UniqueLock* lock)
{
    auto optLock = processUniqueLock(lock);
    assert(mState == EMPTY && "Only empty transfer can be filled, call reset() if you want to refill transfer");

    mUserCallback = std::move(callback);
    mUserData = userData;
    libusb_fill_bulk_transfer(mTransfer,
                              device.mHandle,
                              endpoint,
                              buffer,
                              length,
                              LibusbTransfer::sCallbackWrapper,
                              nullptr,
                              timeout);

    mState = READY;
}

void LibusbTransfer::sCallbackWrapper(libusb_transfer* libusbTransfer)
{
    // Check if call is correct
    assert(libusbTransfer != nullptr);
    if (libusbTransfer->user_data == nullptr)
        return;

    auto transfer = *getTransferFromUserData(libusbTransfer->user_data);
    auto lock = transfer->getUniqueLock();

    const auto& callback = transfer->mUserCallback;
    if (callback) { // if callback is set
        transfer->mState = IN_CALLBACK;
        callback(transfer, lock);
    }

    freeUserData(libusbTransfer->user_data);
    transfer->mState = READY;
}

void* LibusbTransfer::prepareUserData()
{
    return new Pointer(this->shared_from_this());
}

void LibusbTransfer::freeUserData(void* userData)
{
    delete static_cast<Pointer*>(userData);
}

LibusbTransfer::Pointer* LibusbTransfer::getTransferFromUserData(void* userData)
{
    return static_cast<Pointer*>(userData);
}

std::optional<LibusbTransfer::UniqueLock> LibusbTransfer::processUniqueLock(const LibusbTransfer::UniqueLock* lock)
{
    if (lock == nullptr)
        return UniqueLock(mMutex);

    if (isLocked(*lock))
        throw ObjLibusbError(ObjLibusbError::INCORRECT_MUTEX_LOCK);

    return std::nullopt;
}

std::optional<LibusbTransfer::SharedLock> LibusbTransfer::processVariantLock(const VariantLock& lock) const
{
    if (std::holds_alternative<nullptr_t>(lock))
        return SharedLock(mMutex);

    if (!isVariantLocked(lock))
        throw ObjLibusbError(ObjLibusbError::INCORRECT_MUTEX_LOCK);

    return std::nullopt;
}

bool LibusbTransfer::isVariantLocked(const LibusbTransfer::VariantLock& lockPtr) const
{
    if (std::holds_alternative<const UniqueLock*>(lockPtr))
        return isLocked(*std::get<const UniqueLock*>(lockPtr));

    if (std::holds_alternative<const SharedLock*>(lockPtr))
        return isLocked(*std::get<const SharedLock*>(lockPtr));

    return false;
}
