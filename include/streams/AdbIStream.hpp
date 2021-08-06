#ifndef ADB_LIB_ADBISTREAM_HPP
#define ADB_LIB_ADBISTREAM_HPP

#include "streams/AdbStreamBase.hpp"

class AdbIStream
{
public:
    using Base = AdbStreamBase;
    using DevicePointer = Base::DevicePointer;

    AdbIStream& operator>> (std::string& string);
    AdbIStream& operator>> (APayload& payload);

protected:
    friend AdbDevice;
    std::condition_variable mReceived;

    explicit AdbIStream(DevicePointer pointer);

    void received(const APayload& payload);
};

#endif //ADB_LIB_ADBISTREAM_HPP
