#include "streams/AdbStreamBase.hpp"
#include "AdbDevice.hpp"


AdbStreamBase::AdbStreamBase(std::shared_ptr<AdbDevice> pointer)
    : mDevice(std::move(pointer))
{}
