#include "LibusbError.hpp"

LibusbError::LibusbError()
    : LibusbError(LIBUSB_SUCCESS)
{}

LibusbError::LibusbError(ssize_t err)
    : std::runtime_error(libusb_error_name(err))
    , mError(static_cast<libusb_error>(err))
{}

libusb_error LibusbError::getCode() const {
    return mError;
}
