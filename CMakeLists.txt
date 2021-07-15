cmake_minimum_required(VERSION 3.19)
project(adb_test)

set(CMAKE_CXX_STANDARD 17)

set(ADB_SRC src/main.cpp)

add_executable(adb_test ${ADB_SRC})
add_subdirectory(obj-libusb)

target_link_libraries(adb_test obj-libusb)
target_include_directories(adb_test PUBLIC include/ obj-libusb/include/)