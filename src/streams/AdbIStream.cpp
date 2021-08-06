#include "streams/AdbIStream.hpp"

AdbIStream& AdbIStream::operator>>(std::string &string)
{
    std::unique_lock lock(mQueueMutex);
    if (mQueue.empty())
        mReceived.wait(lock, [this] {return !mQueue.empty();});

    string = std::move(mQueue.front().toString());
    mQueue.pop_front();

    return *this;
}

AdbIStream& AdbIStream::operator>> (APayload& payload)
{
    std::unique_lock lock(mQueueMutex);
    if (mQueue.empty())
        mReceived.wait(lock, [this] {return !mQueue.empty();});

    payload = std::move(mQueue.front());
    mQueue.pop_front();

    return *this;
}

AdbIStream::AdbIStream(std::shared_ptr<AdbDevice> pointer)
: AdbStreamBase(std::move(pointer))
{}

void AdbIStream::received(const APayload& payload)
{
    std::unique_lock lock(mQueueMutex);
    mQueue.push_back(payload);
    lock.unlock();
    mReceived.notify_one();
}