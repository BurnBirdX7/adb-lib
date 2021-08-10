#ifndef ADB_LIB_ADBISTREAM_HPP
#define ADB_LIB_ADBISTREAM_HPP

#include "AdbStreamBase.hpp"


class AdbIStream {
public:
    explicit AdbIStream(std::shared_ptr<AdbStreamBase>  basePtr);

    AdbIStream& operator>> (std::string& string);
    AdbIStream& operator>> (APayload& payload);

    bool isOpen();
    void close();

private:
    std::shared_ptr<AdbStreamBase> mBasePtr;

};

#endif //ADB_LIB_ADBISTREAM_HPP
