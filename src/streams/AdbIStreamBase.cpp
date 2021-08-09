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

std::string AdbIStreamBase::getAsString()
{
    return getAsPayload().toString();
}

APayload AdbIStreamBase::getAsPayload()
{
    std::unique_lock lock(mQueueMutex);
    if (mQueue.empty())
        mReceived.wait(lock, [this] {return !mQueue.empty();});

    auto payload = std::move(mQueue.front());
    mQueue.pop_front();
    return payload;
}
