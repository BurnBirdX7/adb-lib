#ifndef ADB_LIB_ADBISTREAMBASE_HPP
#define ADB_LIB_ADBISTREAMBASE_HPP

#include "streams/AdbStreamBase.hpp"

class AdbIStreamBase
        : public AdbStreamBase
{
public:
    std::string getAsString();
    APayload getAsPayload();

    void received(const APayload& payload);


protected:
    friend AdbDevice;
    std::condition_variable mReceived;

    explicit AdbIStreamBase(DevicePointer pointer);

};

#endif //ADB_LIB_ADBISTREAMBASE_HPP
