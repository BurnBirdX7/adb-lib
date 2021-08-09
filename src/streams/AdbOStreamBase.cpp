#include "streams/AdbOStreamBase.hpp"
#include "AdbDevice.hpp"

#include <cassert>


AdbOStreamBase::AdbOStreamBase(AdbOStreamBase::DevicePointer pointer, uint32_t localId, uint32_t remoteId)
    : AdbStreamBase(std::move(pointer))
    , mLocalId(localId)
    , mRemoteId(remoteId)
    , mReady(true)
{}

void AdbOStreamBase::send(APayload&& payload)
{
    if (!mOpen)
        return;

    std::unique_lock lock(mQueueMutex);
    if (mReady && mQueue.empty()) {
        mReady = false;
        mDevice->sendWrite(mLocalId, mRemoteId, std::move(payload));
    }
    else {
        mQueue.emplace_back(std::move(payload));
    }
}

void AdbOStreamBase::ready()
{
    if (!mOpen)
        return;

    std::unique_lock lock(mQueueMutex);
    if (!mQueue.empty()) {
        mDevice->sendWrite(mLocalId, mRemoteId, std::move(mQueue.front()));
        mQueue.pop_front();
        mReady = false;
    }
    else {
        mReady = true;
    }

}