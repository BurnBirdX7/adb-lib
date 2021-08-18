# adb-lib

## Dependencies
 * [libusb/libusb](https://github.com/libusb/libusb)
 * [ARMmbed/mbedTLS](https://github.com/ARMmbed/mbedtls)

ObjLibusb library (from [BurnBirdX7/ObjLibusb](https://github.com/BurnBirdX7/ObjLibusb)) already in project's files

## Build

### Preparation

You need to install dependencies.\
or install them with *apt*:
```shell
sudo apt install libusb-1.0-0-dev
sudo apt install libmbedtls-dev
```

You can install libusb from *vcpkg*:
```shell
vcpkg install libusb
```

You can build libusb and MbedTLS by yourself or download binaries.



### CMake
(in project's root; requires CMake version >= 3.15)
```shell
mkdir build && cd build
cmake ..                # generate project
cmake --build .         # build project
```

#### vcpkg
if you use **vcpkg** you may need to set `CMAKE_TOOLCHAIN_FILE`
to `[vcpkg root]/scripts/buildsystems/vcpkg.cmake` on generation step.\
It will be
```shell
cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake ..
```

#### ...other
If CMake cannot find **libusb**'s package
you can set direct paths via variables `LIBUSB_INCLUDE_DIR` and `LIBUSB_LIBRARY`.\
It will be (*project generation step*)
```shell
cmake -DLIBUSB_INCLUDE_DIR=[path_to_include] -DLIBUSB_LIBRARY=[path_to_binary] ..
```

If CMake cannot find **MbedTLS**'s package
you can set `MBED_TLS_ROOT` variable to the MbedTLS's root directory (directory where you **installed** MbedTLS)\
or you can set direct paths via variables `MBED_TLS_INCLUDE_DIR` and `MBED_TLS_LIBRARIES`:
```shell
cmake -MBED_TLS_ROOT=[path_to_root]
# or
cmake -DMBED_TLS_INCLUDE_DIR=[path_to_include] -DMBED_TLS_LIBRARIES=[list_of_paths_to_library_files] ..
```

## Examples
Build Release version of the library:

### Windows (MSVC)
*(PowerShell)*
```shell
> git clone https://github.com/BurnBirdX7/adb-lib
> cd .\adb-lib\
> mkdir build
> cd .\build\
> cmake -DMBED_TLS_ROOT="C:\Program Files (x86)\mbed TLS" -DCMAKE_TOOLCHAIN_FILE=F:/Dev/vcpkg/scripts/buildsystems/vcpkg.cmake
> cmake --build . --config Release
# to build library only:
> cmake --build . --config Release --target adblib
```

### Linux
```shell
$ git clone https://github.com/BurnBirdX7/adb-lib
$ cd adb-lib/
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ cmake --build .
```
