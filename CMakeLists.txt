cmake_minimum_required(VERSION 3.19)
project(adb_lib)

set(CMAKE_CXX_STANDARD 17)

set(ADBLIB_SRC
        src/APacket.cpp
        src/UsbTransport.cpp
        src/Transport.cpp
        src/APayload.cpp
        src/AdbBase.cpp
        src/Features.cpp
        src/AStream.cpp
        src/AdbDevice.cpp)

add_library(adblib ${ADBLIB_SRC})
add_subdirectory(obj-libusb)

# build
target_link_libraries(adblib obj_libusb)
target_include_directories(adblib PUBLIC include/ obj-libusb/include/)

# test
add_executable(test1 src/test1.cpp)
target_link_libraries(test1 adblib)

