cmake_minimum_required(VERSION 3.19)
project(adb_test)

set(CMAKE_CXX_STANDARD 17)

set(ADBLIB_SRC
        src/APacket.cpp
        src/UsbTransport.cpp
        src/Transport.cpp
        src/APayload.cpp)

add_executable(adbtest ${ADBLIB_SRC} src/main.cpp)
add_library(adblib ${ADBLIB_SRC})
add_subdirectory(obj-libusb)

target_link_libraries(adbtest obj_libusb)
target_link_libraries(adblib obj_libusb)
target_include_directories(adbtest PUBLIC include/ obj-libusb/include/)
target_include_directories(adblib PUBLIC include/ obj-libusb/include/)
