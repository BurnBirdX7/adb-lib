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
        src/AdbDevice.cpp
        src/utils.cpp
        src/streams/AdbStreamBase.cpp
        src/streams/AdbIStream.cpp
        src/streams/AdbOStream.cpp
        )

add_library(adblib ${ADBLIB_SRC})
add_subdirectory(ObjLibusb)

find_package(MbedTLS REQUIRED)

# build
target_link_libraries(adblib PUBLIC
        ObjLibusb
        MbedTLS::mbedtls
        MbedTLS::mbedcrypto
        MbedTLS::mbedx509)
target_include_directories(adblib PUBLIC
        include/
        ObjLibusb/include/)

# tests
add_executable(test_transport tests/test_transport.cpp)
add_executable(test_base tests/test_adb_base.cpp)
add_executable(test_device tests/test_adb_device.cpp)
add_executable(test_utils tests/test_utils.cpp)
target_link_libraries(test_transport adblib)
target_link_libraries(test_base adblib)
target_link_libraries(test_device adblib)
target_link_libraries(test_utils adblib)

