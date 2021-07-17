#include <cassert>
#include <utility>
#include "LibusbTransfer.hpp"
#include "LibusbError.hpp"
#include "LibusbDeviceHandle.hpp"

LibusbTransfer::LibusbTransfer(int isoPacketsNumber)
    : mTransfer(libusb_alloc_transfer(isoPacketsNumber))
    , mState(State::EMPTY)
{
    if (mTransfer == nullptr)
        throw LibusbError(LIBUSB_ERROR_OTHER);
}

LibusbTransfer::~LibusbTransfer()
{
    freeCallbackData();
    libusb_free_transfer(mTransfer);
}

void LibusbTransfer::submit()
{
    assert(mState == READY);
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

    libusb_fill_bulk_transfer(mTransfer,
                              device.mHandle,
                              endpoint,
                              buffer,
                              length,
                              LibusbTransfer::sCallbackWrapper,
                              makeCallbackData(userData, std::move(callback)),
                              timeout);

    mState = READY;
}

void LibusbTransfer::sCallbackWrapper(libusb_transfer* libusbTransfer)
{
    // Check if call is correct
    assert(libusbTransfer != nullptr);
    if (libusbTransfer->user_data == nullptr)
        return;

    auto data = static_cast<CallbackData*>(libusbTransfer->user_data);
    void* userData = data->userData;
    TransferCallback callback = std::move(data->userCallback);
    Pointer transfer = data->transfer;

    transfer->mState = IN_CALLBACK;
    callback(transfer, userData);
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

uint8_t LibusbTransfer::getLength() const
{
    assert(mState >= READY);
    return mTransfer->length;
}

uint8_t LibusbTransfer::getActualLength() const
{
    assert(mState == IN_CALLBACK);
    return mTransfer->actual_length;
}

void* LibusbTransfer::getUserData() const
{
    assert(mState >= READY);
    return getCallbackUserData();
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
    freeCallbackData();
    const auto isoPackets = mTransfer->num_iso_packets;
    libusb_free_transfer(mTransfer);

    mTransfer = libusb_alloc_transfer(isoPackets);
    mState = EMPTY;
}


LibusbTransfer::CallbackData*
LibusbTransfer::makeCallbackData(void* userData, LibusbTransfer::TransferCallback&& callback)
{
    auto data = new CallbackData{
            .userData = userData,
            .userCallback = callback,
            .transfer = shared_from_this()
    };
    return data;
}

void* LibusbTransfer::getCallbackUserData() const
{
    auto data = getCallbackData();
    return data ? data->userData : nullptr;
}

LibusbTransfer::CallbackData* LibusbTransfer::getCallbackData() const
{
    if (!mTransfer)
        return {};
    return static_cast<CallbackData*>(mTransfer->user_data);
}

void LibusbTransfer::freeCallbackData()
{
    auto data = getCallbackData();
    delete data;
    mTransfer->user_data = nullptr;
}




