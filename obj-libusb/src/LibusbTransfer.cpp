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
    libusb_free_transfer(mTransfer);
}

void LibusbTransfer::submit()
{
    assert(mState == FILLED);
    CHECK_ERROR(libusb_submit_transfer(mTransfer));
    mState = SUBMITTED;
}

void LibusbTransfer::cancel()
{
    assert(mState == SUBMITTED);
    CHECK_ERROR(libusb_cancel_transfer(mTransfer));
}
void LibusbTransfer::fillBulk(const LibusbDeviceHandle &device, uint8_t endpoint, unsigned char *buffer, int length,
                              transferCallback callback, void *userData, unsigned int timeout)
{
    assert(mState == EMPTY && "Only empty transfer can be filled");

    auto data = new CallbackData;
    data->userData = userData;
    data->userCallback = std::move(callback);
    libusb_fill_bulk_transfer(mTransfer, device.mHandle, endpoint,
                              buffer, length, LibusbTransfer::sCallbackWrapper,
                              data, timeout);
}

void LibusbTransfer::sCallbackWrapper(libusb_transfer* libusbTransfer)
{
    // Check if call is correct
    assert(libusbTransfer != nullptr);
    if (libusbTransfer->user_data == nullptr)
        return;

    auto data = static_cast<CallbackData*>(libusbTransfer->user_data);
    void* userData = data->userData;
    transferCallback callback = std::move(data->userCallback);
    weakPointer transfer = data->transfer;

    // Cleanup data:
    delete data;
    libusbTransfer->user_data = nullptr;

    // Invoke callback:
    if (!transfer.expired())
        transfer.lock()->mState = IN_CALLBACK;

    callback(transfer, userData);

    if (!transfer.expired())
        transfer.lock()->mState = OUT_OF_CALLBACK;

}

LibusbTransfer::pointer LibusbTransfer::createTransfer(int isoPacketsNumber)
{
    return LibusbTransfer::pointer(new LibusbTransfer(isoPacketsNumber));
}

uint8_t LibusbTransfer::getFlags() const
{
    assert(mState >= FILLED);
    return mTransfer->flags;
}

uint8_t LibusbTransfer::getEndpoint() const
{
    assert(mState >= FILLED);
    return mTransfer->endpoint;
}

uint8_t LibusbTransfer::getType() const
{
    assert(mState >= FILLED);
    return mTransfer->type;
}

uint LibusbTransfer::getTimeout() const
{
    assert(mState >= FILLED);
    return mTransfer->timeout;
}

uint8_t LibusbTransfer::getStatus() const
{
    assert(mState == IN_CALLBACK);
    return mTransfer->status;
}

uint8_t LibusbTransfer::getLength() const
{
    assert(mState >= FILLED);
    return mTransfer->length;
}

uint8_t LibusbTransfer::getActualLength() const
{
    assert(mState == IN_CALLBACK);
    return mTransfer->actual_length;
}

void* LibusbTransfer::getUserData() const
{
    assert(mState >= FILLED);
    return static_cast<CallbackData*>(mTransfer->user_data)->userData;
}

void LibusbTransfer::enableFreeBufferFlag(bool enable)
{
    assert(mState == FILLED);
    const auto flag = Flag::FREE_BUFFER;
    mTransfer->flags &= enable ? flag : ~flag;
}

void LibusbTransfer::enableAddZeroPacketFlag(bool enable)
{
    assert(mState == FILLED);
    const auto flag = Flag::ADD_ZERO_PACKET;
    mTransfer->flags &= enable ? flag : ~flag;
}


