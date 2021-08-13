#ifndef OBJLIBUSB_ERROR_HPP
#define OBJLIBUSB_ERROR_HPP

#include <stdexcept>
#include "ObjLibusb.hpp"


#define THROW_ON_LIBUSB_ERROR(rc)     \
if ((rc) < 0) {             \
    throw Error(rc);  \
}

namespace ObjLibusb {

    class Error
            : public std::runtime_error
    {
    public:
        enum ErrorCode {
            OK = 0,
            LIBUSB_ERROR = 1
        };

        static const std::string ERROR_STRING[2];

    public:
        // Constructs object with SUCCESS code
        Error();
        explicit Error(ErrorCode err);
        explicit Error(ssize_t err);      // Creates LIBUSB_ERROR

        [[nodiscard]] ErrorCode getErrorCode() const;
        [[nodiscard]] libusb_error getLibusbErrorCode() const;

    private:

        struct {
            ErrorCode obj;
            libusb_error libusb;
        } mError;
    };

}

// Alias
using ObjLibusbError = ObjLibusb::Error;

#endif //OBJLIBUSB_ERROR_HPP
