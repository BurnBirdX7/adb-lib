#include <APayload.hpp>
#include <utility>
#include "streams/AdbOStream.hpp"

AdbOStream::AdbOStream(std::shared_ptr<AdbStreamBase> basePtr)
    : mBasePtr(std::move(basePtr))
{}

AdbOStream& AdbOStream::operator<<(APayload payload)
{
    if (mBasePtr)
        mBasePtr->send(std::move(payload));
    return *this;
}

AdbOStream& AdbOStream::operator<<(const std::string_view& string)
{
    if (mBasePtr)
        mBasePtr->send(APayload(string));
    return *this;
}

bool AdbOStream::isOpen()
{
    return mBasePtr && mBasePtr->isOpen();
}

void AdbOStream::close()
{
    mBasePtr->close();
}
