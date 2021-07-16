cmake_minimum_required(VERSION 3.19)
project(adb_test)

set(CMAKE_CXX_STANDARD 17)

set(ADB_SRC
        src/main.cpp
        src/libadb.cpp
        include/APacket.hpp
        include/adb.hpp
        src/APacket.cpp
        include/Transport.hpp include/UsbTransport.hpp src/UsbTransport.cpp src/Transport.cpp)

add_executable(adb_test ${ADB_SRC})
add_subdirectory(obj-libusb)

target_link_libraries(adb_test obj_libusb)
target_include_directories(adb_test PUBLIC include/ obj-libusb/include/)