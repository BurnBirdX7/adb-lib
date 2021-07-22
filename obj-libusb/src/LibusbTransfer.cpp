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

void LibusbTransfer::submit()
{
    assert(mState == READY);
    mTransfer->user_data = new Pointer(shared_from_this()); // will be deleted in callback
    CHECK_ERROR(libusb_submit_transfer(mTransfer));
    mState = SUBMITTED;
}

void LibusbTransfer::cancel()
{
    assert(mState == SUBMITTED);
    CHECK_ERROR(libusb_cancel_transfer(mTransfer));
}
void LibusbTransfer::fillBulk(const LibusbDeviceHandle &device, uint8_t endpoint, unsigned char *buffer, int length,
                              TransferCallback callback, void *userData, unsigned int timeout)
{
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

    auto transfer = *static_cast<LibusbTransfer::Pointer*>(libusbTransfer->user_data);

    const auto& callback = transfer->mUserCallback;
    if (callback) { // if callback is set
        transfer->mState = IN_CALLBACK;
        callback(transfer);
    }

    delete static_cast<Pointer*>(transfer->mTransfer->user_data);
    transfer->mState = READY;
}

LibusbTransfer::Pointer LibusbTransfer::createTransfer(int isoPacketsNumber)
{
    return LibusbTransfer::Pointer(new LibusbTransfer(isoPacketsNumber));
}

uint8_t LibusbTransfer::getFlags() const
{
    assert(mState >= READY);
    return mTransfer->flags;
}

uint8_t LibusbTransfer::getEndpoint() const
{
    assert(mState >= READY);
    return mTransfer->endpoint;
}

uint8_t LibusbTransfer::getType() const
{
    assert(mState >= READY);
    return mTransfer->type;
}

uint LibusbTransfer::getTimeout() const
{
    assert(mState >= READY);
    return mTransfer->timeout;
}

uint8_t LibusbTransfer::getStatus() const
{
    assert(mState == IN_CALLBACK);
    return mTransfer->status;
}

int LibusbTransfer::getLength() const
{
    assert(mState >= READY);
    return mTransfer->length;
}

int LibusbTransfer::getActualLength() const
{
    assert(mState == IN_CALLBACK);
    return mTransfer->actual_length;
}

void* LibusbTransfer::getUserData() const
{
    assert(mState >= READY);
    return mUserData;
}

void LibusbTransfer::enableFreeBufferFlag(bool enable)
{
    assert(mState == READY);
    constexpr auto flag = Flag::FREE_BUFFER;
    if (enable)
        mTransfer->flags |= flag;
    else
        mTransfer->flags &= ~flag;
}

void LibusbTransfer::enableAddZeroPacketFlag(bool enable)
{
    assert(mState == READY);
    constexpr auto flag = Flag::ADD_ZERO_PACKET;
    if (enable)
        mTransfer->flags |= flag;
    else
        mTransfer->flags &= ~flag;
}

LibusbTransfer::State LibusbTransfer::getState() const
{
    return mState;
}

void LibusbTransfer::reset()
{
    assert(mState == READY);
    const auto isoPackets = mTransfer->num_iso_packets;
    libusb_free_transfer(mTransfer);

    mTransfer = libusb_alloc_transfer(isoPackets);
    mState = EMPTY;
}

uint8_t* LibusbTransfer::getBuffer() const
{
    assert(mState >= READY);
    return mTransfer->buffer;
}

void LibusbTransfer::setNewBuffer(unsigned char* buffer, uint8_t length)
{
    assert(mState == READY || mState == IN_CALLBACK);
    mTransfer->buffer = buffer;
    mTransfer->length = length;
}

void LibusbTransfer::setNewUserData(void* userData)
{
    assert(mState == READY || mState == IN_CALLBACK);
    mUserData = userData;
}


