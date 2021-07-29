#ifndef OBJ_LIBUSB__LIBUSBERROR_HPP
#define OBJ_LIBUSB__LIBUSBERROR_HPP

#include <stdexcept>
#include "Libusb.hpp"


#define CHECK_LIBUSB_ERROR(rc)     \
if ((rc) < 0) {             \
    throw LibusbError(rc);  \
}

#define OBJLIBUSB_IOSTREAM_REPORT_ERROR(out, err) (out) << "LibusbError: " << (err).what() << ", code: " << (err).getCode() << std::endl

class LibusbError
        : public std::runtime_error
{
public:
    // Constructs object with SUCCESS code
    LibusbError();
    explicit LibusbError(ssize_t err);

    [[nodiscard]] libusb_error getCode() const;

private:
    libusb_error mError;
};


class ObjLibusbError
        : public std::runtime_error
{
public:
    enum ErrorCode {
        OK = 0,
        INCORRECT_MUTEX_LOCK = 1
    };

public:
    explicit ObjLibusbError(ErrorCode code);
    static const char* OBJLIBUSB_ERROR_CODES[];
    [[nodiscard]] ErrorCode getCode() const;

private:
    ErrorCode mCode;

};




#endif //OBJ_LIBUSB__LIBUSBERROR_HPP
