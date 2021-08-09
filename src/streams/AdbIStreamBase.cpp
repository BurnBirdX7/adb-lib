#include "streams/AdbIStreamBase.hpp"
#include "APayload.hpp"


AdbIStreamBase::AdbIStreamBase(std::shared_ptr<AdbDevice> pointer)
: AdbStreamBase(std::move(pointer))
{}

void AdbIStreamBase::received(const APayload& payload)
{
    std::unique_lock lock(mQueueMutex);
    mQueue.push_back(payload);
    lock.unlock();
    mReceived.notify_one();
}

APayload AdbIStreamBase::getAsPayload()
{
    std::unique_lock lock(mQueueMutex);
    if (mOpen && mQueue.empty())
        mReceived.wait(lock, [this] {return !mQueue.empty();});
    else if (!mOpen)
        return APayload{0};

    auto payload = std::move(mQueue.front());
    mQueue.pop_front();
    return payload;
}
