#ifndef ADB_LIB_ADBISTREAM_HPP
#define ADB_LIB_ADBISTREAM_HPP

#include "AdbIStreamBase.hpp"


class AdbIStream {
public:
    explicit AdbIStream(const std::shared_ptr<AdbIStreamBase>& basePtr);

    AdbIStream& operator>> (std::string& string);
    AdbIStream& operator>> (APayload& payload);

private:
    std::weak_ptr<AdbIStreamBase> mBasePtr;

};

#endif //ADB_LIB_ADBISTREAM_HPP
