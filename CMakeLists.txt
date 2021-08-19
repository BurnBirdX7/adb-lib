cmake_minimum_required(VERSION 3.15)
project(adb_lib)

set(CMAKE_CXX_STANDARD 17)


# Setup

set(ADBLIB_VERSION "1.0.0")

set(source_dir
        src)

set(source
        ${source_dir}/APacket.cpp
        ${source_dir}/UsbTransport.cpp
        ${source_dir}/Transport.cpp
        ${source_dir}/APayload.cpp
        ${source_dir}/AdbBase.cpp
        ${source_dir}/Features.cpp
        ${source_dir}/AdbDevice.cpp
        ${source_dir}/utils.cpp
        ${source_dir}/streams/AdbStreamBase.cpp
        ${source_dir}/streams/AdbIStream.cpp
        ${source_dir}/streams/AdbOStream.cpp)

set(headers_dir
        include)

set(headers
        ${headers_dir}/AdbBase.hpp
        ${headers_dir}/AdbDevice.hpp
        ${headers_dir}/adb.hpp
        ${headers_dir}/AdbStreams.hpp
        ${headers_dir}/APacket.hpp
        ${headers_dir}/APayload.hpp
        ${headers_dir}/Features.hpp
        ${headers_dir}/Transport.hpp
        ${headers_dir}/UsbTransport.hpp
        ${headers_dir}/utils.hpp

        ${headers_dir}/streams/AdbIStream.hpp
        ${headers_dir}/streams/AdbOStream.hpp
        ${headers_dir}/streams/AdbStreamBase.hpp)

set(cmake_config
        cmake/adblib-config.cmake)

set(lib_destination
        "lib/adblib-${ADBLIB_VERSION}")

set(headers_destination
        "include/adblib-${ADBLIB_VERSION}")

set(cmake_destination
        ${lib_destination})


add_library(adblib
        ${source})

# ! Setup


# Build
# Include

target_include_directories(adblib PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${headers_dir}>
        $<INSTALL_INTERFACE:${headers_destination}>)
# ! Include

# Mbed TLS
if (DEFINED MBED_TLS_ROOT)
    set(CMAKE_PREFIX_PATH "${MBED_TLS_ROOT};${CMAKE_PREFIX_PATH}")
    message(NOTICE "MBED_TLS_ROOT added to CMake prefix")
endif()

find_package(MbedTLS 3.0.0)
if(MbedTLS_FOUND)
    message(NOTICE "MbedTLS found")

    target_link_libraries(adblib PUBLIC
            MbedTLS::mbedtls
            MbedTLS::mbedcrypto
            MbedTLS::mbedx509)

else()
    message(NOTICE "MbedTLS not found. Trying to link MbedTLS from provided paths...")

    if ((MBED_TLS_INCLUDE_DIR STREQUAL "") OR (MBED_TLS_LIBRARIES STREQUAL ""))
        message(SEND_ERROR "MbedTLS paths are not set")
    else()
        message(NOTICE "MbedTLS include dir: ${MBED_TLS_INCLUDE_DIR}")
        message(NOTICE "MbedTLS libraries dir: ${MBED_TLS_LIBRARIES}")

        target_link_libraries(adblib PUBLIC "${MBED_TLS_LIBRARIES}")

        target_include_directories(adblib PUBLIC
                ${MBED_TLS_INCLUDE_DIR})
    endif()

endif()
# ! Mbed TLS


# ObjLibusb
if (DEFINED OBJLIBUSB_ROOT)
    set(CMAKE_PREFIX_PATH "${OBJLIBUSB_ROOT};${CMAKE_PREFIX_PATH}")
    message(NOTICE "OBJLIBUSB_ROOT added to CMake prefix")
endif()

find_package(ObjLibusb)
if (ObjLibusb_FOUND)
    message(NOTICE "ObjLibusb found")

    if(DEFINED UNIX)
        find_package(Threads REQUIRED)
    endif()

    target_link_libraries(adblib PUBLIC ObjLibusb)
else()
    message(NOTICE "ObjLibusb not found. Trying to link ObjLibusb from provided paths")

    if ((OBJLIBUSB_INCLUDE_DIR STREQUAL "") OR (OBJLIBUSB_LIBRARY STREQUAL ""))
        message(SEND_ERROR "ObjLibusb paths are not set")
    else()
        message(NOTICE "ObjLibusb include dir: ${OBJLIBUSB_INCLUDE_DIR}")
        message(NOTICE "ObjLibusb libraries dir: ${OBJLIBUSB_LIBRARY}")

        target_link_libraries(adblib PUBLIC
                ${OBJLIBUSB_LIBRARY})

        target_include_directories(adblib PUBLIC
                ${OBJLIBUSB_INCLUDE_DIR})
    endif()

endif()
# ! ObjLibusb
# ! Build

# Install

install(TARGETS adblib EXPORT adblib DESTINATION "${lib_destination}/${CMAKE_BUILD_TYPE}")
install(EXPORT adblib DESTINATION "${lib_destination}/${CMAKE_BUILD_TYPE}")

install(FILES ${headers} DESTINATION "${headers_destination}")   # headers
install(FILES ${cmake_config} DESTINATION ${cmake_destination})  # package

# ! Install


# Tests

add_executable(test_transport tests/test_transport.cpp)
add_executable(test_base tests/test_adb_base.cpp)
add_executable(test_device tests/test_adb_device.cpp)
add_executable(test_shell tests/test_adb_shell.cpp)
add_executable(test_utils tests/test_utils.cpp)
target_link_libraries(test_transport adblib)
target_link_libraries(test_base adblib)
target_link_libraries(test_device adblib)
target_link_libraries(test_shell adblib)
target_link_libraries(test_utils adblib)

# ! Tests
