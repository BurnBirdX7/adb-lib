cmake_minimum_required(VERSION 3.15)
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

if (MSVC) # MSVC

    set(LIBUSB_INCLUDE_DIRS)
    set(LIBUSB_LIBRARIES)
    set(VCPKG_ROOT "_" CACHE PATH "Path to vcpkg's root, required if you want to use packages installed by vcpkg")

    message(NOTICE "Trying to find libusb...")
    find_package(libusb CONFIG)

    if (NOT ${libusb_FOUND})
        message(NOTICE "libusb not found with find_package()...")
        message(NOTICE "checking supplied variables...")

        set(LIBUSB_INCLUDE_DIRS ${LIBUSB_INCLUDE_DIR})
        set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARY})
    endif()

    if ((LIBUSB_INCLUDE_DIRS STREQUAL "") OR ("LIBUSB_LIBRARIES" STREQUAL ""))
        message(SEND_ERROR "libusb paths are not set")
    else()
        if (EXISTS "${LIBUSB_INCLUDE_DIRS}/libusb.h")
            message("Strange vcpkg libusb include path detected...")
            set(LIBUSB_INCLUDE_DIRS "${LIBUSB_INCLUDE_DIRS}/../")
        endif()

        message(NOTICE "libusb include dirs: ${LIBUSB_INCLUDE_DIRS}")
        message(NOTICE "libusb library dirs: ${LIBUSB_LIBRARIES}")
    endif()

    target_include_directories(ObjLibusb PUBLIC include ${LIBUSB_INCLUDE_DIRS})
    target_link_libraries(ObjLibusb PUBLIC ${LIBUSB_LIBRARIES})

else() # NOT MSVC

    find_package(Threads REQUIRED)
    target_link_libraries(ObjLibusb PUBLIC usb-1.0 Threads::Threads)
    target_include_directories(ObjLibusb PUBLIC include)

endif()
