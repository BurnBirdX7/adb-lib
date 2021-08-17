# adb-lib

## Dependencies
 * [libusb/libusb](https://github.com/libusb/libusb)
 * [ARMmbed/mbedTLS](https://github.com/ARMmbed/mbedtls)

ObjLibusb library (from [BurnBirdX7/ObjLibusb](https://github.com/BurnBirdX7/ObjLibusb)) already in project's files

## Build (Linux only*)
*for now*

### Preparation

You need to build and install dependencies.\
or install them with *apt*:
```shell
sudo apt install libusb-1.0-0-dev
sudo apt install libmbedtls-dev
```

### CMake
(in project's root; requires CMake version >= 3.15)
```shell
mkdir build && cd build
cmake ..
cmake --build .
```