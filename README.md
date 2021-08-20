# adb-lib

## Dependencies
 * [BurnBirdX7/ObjLibusb](https://github.com/BurnBirdX7/ObjLibusb)
 * * [libusb/libusb](https://github.com/libusb/libusb) (version 1.0.23)
 * [ARMmbed/mbedTLS](https://github.com/ARMmbed/mbedtls) (version 3.0.0)

You need CMake (version >= 3.15) to build the project.

## Preparation

 * Build (and install) ObjLibusb
 * Build (and install) Mbed TLS or acquire it any other way
 
*libusb for Objlibusb*

### apt:
```shell
sudo apt install libusb-1.0-0-dev
sudo apt install libmbedtls-dev
```

### *vcpkg*:
```shell
vcpkg install libusb
vcpkg install mbedtls
```
### Any other way...

You have to build ObjLibusb by yourself.


*If installing with **apt** or **vcpkg** check if versions of packages meet requirements**

## Build
(in project's root)
```shell
mkdir build && cd build
cmake ..                # generate project
cmake --build .         # build project
```

### if you use vcpkg...
... you may need to set `CMAKE_TOOLCHAIN_FILE`
to `[vcpkg root]/scripts/buildsystems/vcpkg.cmake` on generation step.\
It will be
```shell
cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake ..
```

### CMake cannot find packages
On project generation step:

**ObjLibusb**\
you can set `OBJLIBUSB_ROOT` variable to the ObjLibusb's root directory (directory where you **installed** ObjLibusb)\
or you can set direct paths via variables `OBJLIBUSB_INCLUDE_DIR` and `OBJLIBUSB_LIBRARY`:
```shell
cmake -DOBJLIBUSB_ROOT=[path_to_root] ..
# or
cmake -DOBJLIBUSB_INCLUDE_DIR=[path_to_include] -DOBJLIBUSB_LIBRARY=[path_to_binary] ..
```

**MbedTLS**\
you can set `MBED_TLS_ROOT` variable to the MbedTLS's root directory (directory where you **installed** MbedTLS)\
or you can set direct paths via variables `MBED_TLS_INCLUDE_DIR` and `MBED_TLS_LIBRARIES`:
```shell
cmake -DMBED_TLS_ROOT=[path_to_root] ..
# or
cmake -DMBED_TLS_INCLUDE_DIR=[path_to_include] -DMBED_TLS_LIBRARIES=[list_of_paths_to_library_files] ..
```

## Install
```shell
cmake --install .
```

Library will be installed at `CMAKE_INSATALL_PREFIX`.\
Default is `/usr/local` on UNIX systems,\
`C:/Program Files/${PROJECT_NAME}` (or `Program Files (x86)`) on Windows.

Paths to all installed files will be listed in `install_manifest.txt` file in the build directory.

## Use
```cmake
# CMakeLists.txt of your project
find_package(adblib REQUIRED)
target_link_libraries(MyTarget PUBLIC adblib)
```

In some situations CMake requires you to find (with `find_package()`) consequently linked libraries.\
It should not be the case if you linked library with direct path to it.
```cmake
find_package(adblib REQUIRED)
find_package(Threads REQUIRED)
find_package(MbedTLS REQUIRED)
target_link_libraries(MyTarget PUBLIC adblib)
```

## Examples
Build Release version of the library:

### Windows (MSVC)
*(PowerShell)*
 * **mbedtls** built from source and installed
 * **libusb** installed via vcpkg
 * **ObjLibusb** built and installed with CMake
```shell
> git clone https://github.com/BurnBirdX7/adb-lib
> cd .\adb-lib\
> mkdir build
> cd .\build\
> cmake -DMBED_TLS_ROOT="C:\Program Files\mbed TLS" -DOBJLIBUSB_ROOT="C:\Program Files\ObjLibusb" ..
> cmake --build . --config Release
# to build library only:
> cmake --build . --config Release --target adblib
```

### Linux
* **libusb** and **mbedtls** installed via apt
* **ObjLibusb** built and installed with CMake
```shell
$ git clone https://github.com/BurnBirdX7/adb-lib
$ cd adb-lib/
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ cmake --build .
```
