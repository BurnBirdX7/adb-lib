cmake_minimum_required(VERSION 3.19)
project(obj_libusb)

set(CMAKE_CXX_STANDARD 17)
set(OBJ_LIBUSB_SRC
        src/LibusbDevice.cpp
        src/LibusbContext.cpp
        src/LibusbDeviceList.cpp
        src/LibusbError.cpp
        src/LibusbDeviceHandle.cpp
        )

add_library(obj_libusb ${OBJ_LIBUSB_SRC})

find_package(Threads REQUIRED)
target_link_libraries(obj_libusb PUBLIC usb-1.0 Threads::Threads)
target_include_directories(obj_libusb PUBLIC include)