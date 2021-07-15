#ifndef OBJ_LIBUSB__LIBUSBERROR_HPP
#define OBJ_LIBUSB__LIBUSBERROR_HPP

#include <stdexcept>
#include "Libusb.hpp"


#define CHECK_ERROR(rc)     \
if ((rc) < 0) {             \
    throw LibusbError(rc);  \
}


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


#endif //OBJ_LIBUSB__LIBUSBERROR_HPP
