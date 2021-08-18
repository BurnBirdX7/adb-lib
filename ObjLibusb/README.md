# ObjLibusb

Objective Wrapper for libusb

## Dependencies
 * [libusb(github)](https://github.com/libusb/libusb)

You need to install libusb

### apt
```shell
sudo apt install libusb-1.0-0-dev
```

### vcpkg
```shell
vcpkg install libusb
```

### ...other
You can install libusb any other way

## Preparation

Clone project's repository and go to its root.
```shell
git clone https://github.com/BurnBirdX7/ObjLibusb.git
cd ObjLibusb
```

## Build

```shell
mkdir build && cd build   # make dior for build and go there
cmake ..                  # generate project
cmake --build .           # build
```

### vcpkg
if you use **vcpkg** you may need to set `CMAKE_TOOLCHAIN_FILE`
to `[vcpkg root]/scripts/buildsystems/vcpkg.cmake` on generation step.\
It will be
```shell
cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake ..
```

### ...other
if CMake cannot find the library's package
you can set direct paths via variables `LIBUSB_INCLUDE_DIR` and `LIBUSB_LIBRARY`.\
It will be (*project generation step*)
```shell
cmake -DLIBUSB_INCLUDE_DIR=[path_to_include] -DLIBUSB_LIBRARY=[path_to_binary] ..
```