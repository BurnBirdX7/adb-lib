#include "AStream.hpp"
#include "AdbDevice.hpp"


AStream::AStream(std::shared_ptr<AdbDevice> pointer)
    : mDevice(std::move(pointer))
{}

void AStream::enqueue(APayload payload)
{
    mQueue.emplace_back(std::move(payload));
}

APayload AStream::dequeue()
{
    auto payload = std::move(mQueue.front());
    mQueue.pop_front();
    return payload;
}

AIStream& AIStream::operator>>(std::string &string)
{
    std::unique_lock lock(mQueueMutex);
    if (mQueue.empty())
        mReceived.wait(lock, [this] {return !mQueue.empty();});

    string = std::move(mQueue.front().toString());
    mQueue.pop_front();

    return *this;
}

AIStream& AIStream::operator>> (APayload& payload)
{
    std::unique_lock lock(mQueueMutex);
    if (mQueue.empty())
        mReceived.wait(lock, [this] {return !mQueue.empty();});

    payload = std::move(mQueue.front());
    mQueue.pop_front();

    return *this;
}

AIStream::AIStream(std::shared_ptr<AdbDevice> pointer)
    : AStream(std::move(pointer))
{}

void AIStream::received(const APayload& payload)
{
    std::unique_lock lock(mQueueMutex);
    mQueue.push_back(payload);
    lock.unlock();
    mReceived.notify_one();
}

AOStream& AOStream::operator<<(const APayload& payload)
{
    mQueue.push_back(payload);
    send(APayload(payload));
    return *this;
}

AOStream& AOStream::operator<<(APayload&& payload)
{
    send(std::move(payload));
    return *this;
}

AOStream::AOStream(DevicePointer pointer, uint32_t localId, uint32_t remoteId)
    : AStream(std::move(pointer))
    , mLocalId(localId)
    , mRemoteId(remoteId)
    , mReady(true)
{}

void AOStream::send(APayload&& payload)
{
    std::unique_lock lock(mQueueMutex);
    if (mReady && mQueue.empty()) {
        mReady = false;
        mDevice->sendWrite(mLocalId, mRemoteId, std::move(payload));
    }
    else {
        mQueue.emplace_back(std::move(payload));
    }
}

void AOStream::ready()
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
