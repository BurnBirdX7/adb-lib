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
        src/AdbDevice.cpp
        src/utils.cpp)

add_library(adblib ${ADBLIB_SRC})
add_subdirectory(obj-libusb)

# build
target_link_libraries(adblib obj_libusb)
target_include_directories(adblib PUBLIC include/ obj-libusb/include/)

# tests
add_executable(test_transport src/test_transport.cpp)
add_executable(test_base src/test_adb_base.cpp)
add_executable(test_device src/test_adb_device.cpp)
add_executable(test_utils src/test_utils.cpp)
target_link_libraries(test_transport adblib)
target_link_libraries(test_base adblib)
target_link_libraries(test_device adblib)
target_link_libraries(test_utils adblib)

