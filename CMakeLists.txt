cmake_minimum_required(VERSION 3.19)
project(adb_test)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(obj-libusb)

add_executable(adb_test main.cpp)
target_link_libraries(adb_test obj-libusb)

target_include_directories(adb_test PUBLIC obj-libusb/include)