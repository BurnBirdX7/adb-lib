#ifndef ADB_LIB_ADBOSTREAM_HPP
#define ADB_LIB_ADBOSTREAM_HPP

#include "AdbStreamBase.hpp"

class AdbOStream
{
public:
    explicit AdbOStream(std::shared_ptr<AdbStreamBase>  basePtr);

    AdbOStream& operator<< (const std::string_view& string);
    AdbOStream& operator<< (APayload payload);

    bool isOpen();
    void close();

private:
    std::shared_ptr<AdbStreamBase> mBasePtr;

};

#endif //ADB_LIB_ADBOSTREAM_HPP
