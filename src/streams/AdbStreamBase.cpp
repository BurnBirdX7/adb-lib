#include "streams/AdbStreamBase.hpp"
#include "AdbDevice.hpp"


AdbStreamBase::AdbStreamBase(std::shared_ptr<AdbDevice> pointer)
    : mDevice(std::move(pointer))
    , mOpen(true)
{}

void AdbStreamBase::close()
{
    mOpen = false;
}

bool AdbStreamBase::isOpen() const
{
    return mOpen;
}
