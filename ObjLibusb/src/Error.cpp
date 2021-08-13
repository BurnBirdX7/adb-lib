#include "ObjLibusb/Error.hpp"

namespace ObjLibusb {

    const std::string Error::ERROR_STRING[2] = {
            "OK",
            "Libusb Error"
    };

    Error::Error()
        : Error(LIBUSB_SUCCESS)
    {}

    Error::Error(ssize_t err)
        : std::runtime_error(ERROR_STRING[LIBUSB_ERROR] + ": " + libusb_error_name(err))
        , mError()
    {
        mError.obj = LIBUSB_ERROR;
        mError.libusb = static_cast<libusb_error>(err);
    }

    Error::Error(Error::ErrorCode err)
        : std::runtime_error("ObjLibusb Error: " + ERROR_STRING[err])
        , mError()
    {
        mError.obj = err;
        mError.libusb = LIBUSB_SUCCESS;
    }


    Error::ErrorCode Error::getErrorCode() const {
        return mError.obj;
    }

    libusb_error Error::getLibusbErrorCode() const {
        return mError.libusb;
    }

}