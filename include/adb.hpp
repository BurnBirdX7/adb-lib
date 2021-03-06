#ifndef ADB_LIB_ADB_HPP
#define ADB_LIB_ADB_HPP

#include <cstdint>

constexpr size_t MAX_PAYLOAD_V1 = 4 * 1024;
constexpr size_t MAX_PAYLOAD = 1024 * 1024;
constexpr size_t MAX_FRAMEWORK_PAYLOAD = 64 * 1024;

constexpr size_t LINUX_MAX_SOCKET_SIZE = 4194304;

#define A_SYNC 0x434e5953
#define A_CNXN 0x4e584e43
#define A_OPEN 0x4e45504f
#define A_OKAY 0x59414b4f
#define A_CLSE 0x45534c43
#define A_WRTE 0x45545257
#define A_AUTH 0x48545541
#define A_STLS 0x534C5453

#define A_VERSION_MIN 0x01000000
#define A_VERSION_SKIP_CHECKSUM 0x01000001
#define A_VERSION 0x01000001

#define ADB_CLASS 0xff
#define ADB_SUBCLASS 0x42
#define ADB_PROTOCOL 0x1

#endif //ADB_LIB_ADB_HPP
