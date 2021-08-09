#ifndef ADB_LIB_ADBOSTREAMBASE_HPP
#define ADB_LIB_ADBOSTREAMBASE_HPP

#include <memory>
#include <mutex>

#include "streams/AdbStreamBase.hpp"
#include "APayload.hpp"

class AdbOStreamBase
        : public AdbStreamBase
{
public:

    void send(APayload&& payload);

protected:
    friend AdbDevice;

    AdbOStreamBase(DevicePointer pointer, uint32_t localId, uint32_t remoteId);

    void ready();

private:
    bool mReady;
    uint32_t mLocalId;
    uint32_t mRemoteId;

};


#endif //ADB_LIB_ADBOSTREAMBASE_HPP
