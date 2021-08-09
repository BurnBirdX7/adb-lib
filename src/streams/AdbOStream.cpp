#include "streams/AdbOStream.hpp"


AdbOStream::AdbOStream(const std::shared_ptr<AdbOStreamBase>& basePtr)
    : mBasePtr(basePtr)
{}

AdbOStream& AdbOStream::operator<<(APayload payload)
{
    auto shared = mBasePtr.lock();
    if (shared)
        shared->send(std::move(payload));

    return *this;
}

AdbOStream& AdbOStream::operator<<(const std::string_view& string)
{
    auto shared = mBasePtr.lock();
    if (shared)
        shared->send(APayload(string));

    return *this;
}
