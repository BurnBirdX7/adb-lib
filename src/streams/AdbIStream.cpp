#include "streams/AdbIStream.hpp"
#include "APayload.hpp"

AdbIStream& AdbIStream::operator>>(std::string &string)
{
    auto shared = mBasePtr.lock();
    if (shared)
        string = std::move(shared->getAsString());

    return *this;
}

AdbIStream& AdbIStream::operator>> (APayload& payload)
{
    auto shared = mBasePtr.lock();
    if (shared)
        payload = std::move(shared->getAsPayload());

    return *this;
}

AdbIStream::AdbIStream(const std::shared_ptr<AdbIStreamBase>& basePtr)
    : mBasePtr(basePtr)
{}
