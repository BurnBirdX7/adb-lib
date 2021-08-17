#include "streams/AdbIStream.hpp"

#include <utility>
#include "APayload.hpp"


AdbIStream::AdbIStream(std::shared_ptr<AdbStreamBase> basePtr)
: mBasePtr(std::move(basePtr))
{}

AdbIStream& AdbIStream::operator>>(std::string &string)
{
    string = std::move(mBasePtr->getPayload().toString());
    return *this;
}

AdbIStream& AdbIStream::operator>> (APayload& payload)
{
    payload = std::move(mBasePtr->getPayload());
    return *this;
}

bool AdbIStream::isOpen() const
{
    return mBasePtr && mBasePtr->isOpen();
}

void AdbIStream::close()
{
    mBasePtr.reset();
}

bool AdbIStream::isEmpty() const
{
    return mBasePtr->mIncomingQueue.empty();
}
