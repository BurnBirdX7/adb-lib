#ifndef ADB_LIB_ADBOSTREAM_HPP
#define ADB_LIB_ADBOSTREAM_HPP

#include <memory>
#include <mutex>

#include "streams/AdbOStream.hpp"
#include "APayload.hpp"

class AdbOStream
{
public:
    using Base = AdbStreamBase;
    using DevicePointer = Base::DevicePointer;

    AdbOStream& operator<< (const std::string_view& string);
    AdbOStream& operator<< (const APayload& payload);
    AdbOStream& operator<< (APayload&& payload);

protected:
    friend AdbDevice;

    AdbOStream(DevicePointer pointer, uint32_t localId, uint32_t remoteId);

    void ready();

private:
    bool mReady;
    uint32_t mLocalId;
    uint32_t mRemoteId;

    void send(APayload&& payload);

    std::std::weak_ptr<Base> mBase;

};


#endif //ADB_LIB_ADBOSTREAM_HPP
