# Build MacOS

*Last Updated on October 19, 2021*

Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. This will include the necessary environment variables to customize your build. Only macOS specific instructions are found in this document.

## Prerequisites

### CMake, OpenSSL, and NPM

[Homebrew](https://brew.sh/) is an excellent package manager for macOS. It makes the installation of some Vircadia dependencies very simple.

```bash
brew install cmake openssl npm
```

**Note:** You can also download alternative CMake versions from [Github](https://github.com/Kitware/CMake/releases) if needed.

### Python 3

Download an install Python 3.6.6 or higher from [here](https://www.python.org/downloads/).
Execute the `Update Shell Profile.command` script that is provided with the installer.

### MacOS SDK

You will need version `10.12` of the macOS SDK for building, otherwise you may have crashing or other unintended issues due to the deprecation of OpenGL on macOS. You can get that SDK from [here](https://github.com/phracker/MacOSX-SDKs). You must copy it in to your Xcode SDK directory, e.g.

```bash
cp -rp ~/Downloads/MacOSX10.12.sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/
```

### OpenSSL

Assuming you've installed OpenSSL using the homebrew instructions above, you'll need to set `OPENSSL_ROOT_DIR` so CMake can find your installations.
For OpenSSL installed via homebrew, set `OPENSSL_ROOT_DIR` via `export OPENSSL_ROOT_DIR=/usr/local/opt/openssl` or by appending `-DOPENSSL_ROOT_DIR=/usr/local/opt/openssl` to `cmake`.

## Generate and Build

You can choose to use either Unix Makefiles or Xcode.

### Xcode

You can ask CMake to generate Xcode project files instead of Unix Makefiles using the `-G Xcode` parameter after CMake. You will need to select the Xcode installation in the terminal first if you have not done so already.

```bash
sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer

cmake ../ -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.12 -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -G Xcode -DOSX_SDK=10.12  ..
```

After running CMake, you will have the make files or Xcode project file necessary to build all of the components. Open the `vircadia.xcodeproj` file, choose `ALL_BUILD` from the Product > Scheme menu (or target drop down), and click Run.

If the build completes successfully, you will have built targets for all components located in the `build/${target_name}/Debug` directories.

### make

Run CMake.

```bash
cmake -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.12 -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOSX_SDK=10.12  ..
```

You can append `-j4` to assign more threads to build with. The number indicates the number of threads, e.g. 4.

To package the installation, you can simply run `make package` afterwards.

## Notes

If build is intended for packaging or creation of AppImage, `VIRCADIA_CPU_ARCHITECTURE`
CMake variable needs to be set to architecture specific value.
It defaults to `-march=native -mtune=native`, which yields builds optimized for particular
machine, but builds will not work on machines lacking same CPU instructions.
For packaging and AppImage it is recommended to set it to different value, for example `-msse3`.
Setting `VIRCADIA_CPU_ARCHITECTURE` to empty string will use default compiler settings and yield
maximum compatibility.

## FAQ

1. **Problem:** Running the scheme `interface.app` from Xcode causes a crash for Interface related to `libgl`.
    1. **Cause:** The target `gl` generates a binary called `libgl`. A macOS `libGL.framework` item gets loaded instead by Xcode.
    2. **Solution:** In the Xcode target settings for `libgl`, set the version to `1.0.0`.
2. **Problem:** CMake complains about Python 3 being missing.
    1. **Cause:** CMake might be out of date.
    2. **Solution:** Try updating your CMake binary with command `brew upgrade cmake`, or by downloading and running a newer CMake installer, depending on how you originally installed CMake. Please keep in mind the recommended CMake versions noted above.
