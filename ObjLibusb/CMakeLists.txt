cmake_minimum_required(VERSION 3.19)
project(ObjLibusb)


message(${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 17)

set(OBJLIBUSB_SRC
        src/Device.cpp
        src/Context.cpp
        src/DeviceList.cpp
        src/Error.cpp
        src/DeviceHandle.cpp
        src/Transfer.cpp)

add_library(ObjLibusb ${OBJLIBUSB_SRC})

find_package(Threads REQUIRED)
target_link_libraries(ObjLibusb PUBLIC usb-1.0 Threads::Threads)
target_include_directories(ObjLibusb PUBLIC include)
