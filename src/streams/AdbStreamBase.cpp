#include "streams/AdbStreamBase.hpp"
#include "AdbDevice.hpp"


AdbStreamBase::AdbStreamBase(std::weak_ptr<AdbDevice> pointer, uint32_t localId, uint32_t remoteId)
    : mDevice(std::move(pointer))
    , mIsOpen(true)
    , mLocalId(localId)
    , mRemoteId(remoteId)
{}

void AdbStreamBase::close()
{
    mIsOpen = false;
}

bool AdbStreamBase::isOpen() const
{
    return mIsOpen && !mDevice.expired();
}

void AdbStreamBase::received(const APayload& payload)
{
    if (!isOpen())
        return;

    std::unique_lock lock(mIncomingMutex);
    mIncomingQueue.push_back(payload);
    lock.unlock();
    mReceived.notify_one();
}

APayload AdbStreamBase::getPayload()
{
    std::unique_lock lock(mIncomingMutex);
    if (isOpen() && mIncomingQueue.empty())
        mReceived.wait(lock, [this] {return !mIncomingQueue.empty();});
    else if (!isOpen())
        return APayload{0};

    auto payload = std::move(mIncomingQueue.front());
    mIncomingQueue.pop_front();
    return payload;
}

void AdbStreamBase::send(APayload&& payload)
{
    auto device = lockDeviceIfOpen();
    if (!device)
        return;

    std::unique_lock lock(mOutgoingMutex);
    if (mReadyToSend && mOutgoingQueue.empty()) {
        mReadyToSend = false;
        device->send(mLocalId, mRemoteId, std::move(payload));
    }
    else {
        mOutgoingQueue.emplace_back(std::move(payload));
    }
}

void AdbStreamBase::readyToSend()
{
    auto device = lockDeviceIfOpen();
    if (!device)
        return;

    std::unique_lock lock(mOutgoingMutex);
    if (!mOutgoingQueue.empty()) {
        device->send(mLocalId, mRemoteId, std::move(mOutgoingQueue.front()));
        mOutgoingQueue.pop_front();
        mReadyToSend = false;
    }
    else {
        mReadyToSend = true;
    }
}

AdbStreamBase::SharedDevice AdbStreamBase::lockDeviceIfOpen()
{
    if (mIsOpen)
        return mDevice.lock();
    return {};
}

AdbStreamBase::~AdbStreamBase()
{
    auto device = mDevice.lock();
    if (device)
        device->closeStream(mLocalId);
}