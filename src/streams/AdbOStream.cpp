#include "streams/AdbOStream.hpp"


AdbOStream& AdbOStream::operator<<(const APayload& payload)
{
    mQueue.push_back(payload);
    send(APayload(payload));
    return *this;
}

AdbOStream& AdbOStream::operator<<(APayload&& payload)
{
    send(std::move(payload));
    return *this;
}

AdbOStream::AdbOStream(AdbOStream::DevicePointer pointer, uint32_t localId, uint32_t remoteId)
: AStream(std::move(pointer))
, mLocalId(localId)
, mRemoteId(remoteId)
, mReady(true)
{}

void AdbOStream::send(APayload&& payload)
{
    std::unique_lock lock(mBase->mQueueMutex);
    if (mReady && mQueue.empty()) {
        mReady = false;
        mDevice->sendWrite(mLocalId, mRemoteId, std::move(payload));
    }
    else {
        mQueue.emplace_back(std::move(payload));
    }
}

void AdbOStream::ready()
{
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
