cmake_minimum_required(VERSION 3.15)
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

# build

if (DEFINED MBED_TLS_ROOT)
    set(CMAKE_PREFIX_PATH "${MBED_TLS_ROOT};${CMAKE_PREFIX_PATH}")
    message(NOTICE "MBED_TLS_ROOT added to CMake prefix")
endif()

find_package(MbedTLS)
if(MbedTLS_FOUND)

    target_link_libraries(adblib PUBLIC
            ObjLibusb
            MbedTLS::mbedtls
            MbedTLS::mbedcrypto
            MbedTLS::mbedx509)

    target_include_directories(adblib PUBLIC
            include/
            ObjLibusb/include/)

else()
    message(NOTICE "Trying to link MbedTLS from provided paths")

    if ((MBED_TLS_INCLUDE_DIR STREQUAL "") OR (MBED_TLS_LIBRARIES STREQUAL ""))
        message(SEND_ERROR "MbedTLS paths are not set")
    else()
        message(NOTICE "MbedTLS include dir: ${MBED_TLS_INCLUDE_DIR}")
        message(NOTICE "MbedTLS libraries dir: ${MBED_TLS_LIBRARIES}")

        target_link_libraries(adblib PUBLIC "ObjLibusb;${MBED_TLS_LIBRARIES}")

        target_include_directories(adblib PUBLIC
                include/
                ObjLibusb/include/
                ${MBED_TLS_INCLUDE_DIR})
    endif()

endif()

# tests
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

