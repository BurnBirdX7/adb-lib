#ifndef ADB_LIB_ADBOSTREAM_HPP
#define ADB_LIB_ADBOSTREAM_HPP

#include "AdbOStreamBase.hpp"


class AdbOStream
{
public:
    explicit AdbOStream(const std::shared_ptr<AdbOStreamBase>& basePtr);

    AdbOStream& operator<< (const std::string_view& string);
    AdbOStream& operator<< (APayload payload);

private:
    std::weak_ptr<AdbOStreamBase> mBasePtr;

};

#endif //ADB_LIB_ADBOSTREAM_HPP
