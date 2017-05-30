Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only Android specific instructions are found in this file.

### Android Dependencies

You will need the following tools to build our Android targets.

* [cmake](http://www.cmake.org/download/) ~> 3.5.1
* [Qt](http://www.qt.io/download-open-source/#) ~> 5.6.2
* [ant](http://ant.apache.org/bindownload.cgi) ~> 1.9.4
* [Android NDK](https://developer.android.com/tools/sdk/ndk/index.html) ~> r10d
* [Android SDK](http://developer.android.com/sdk/installing/index.html) ~> 24.4.1.1
  * Install the latest Platform-tools
  * Install the latest Build-tools
  * Install the SDK Platform for API Level 19
  * Install Sources for Android SDK for API Level 19
  * Install the ARM EABI v7a System Image if you want to run an emulator.

You will also need to cross-compile the dependencies required for all platforms for Android, and help CMake find these compiled libraries on your machine.

#### Scribe

High Fidelity has a shader pre-processing tool called `scribe` that various libraries will call on during the build process. You must compile scribe using your native toolchain (following the build instructions for your platform) and then pass a CMake variable or set an ENV variable `SCRIBE_PATH` that is a path to the scribe executable.

CMake will fatally error if it does not find the scribe executable while using the android toolchain.

#### Optional Components

* [Oculus Mobile SDK](https://developer.oculus.com/downloads/#sdk=mobile) ~> 0.4.2

#### ANDROID_LIB_DIR

Since you won't be installing Android dependencies to system paths on your development machine, CMake will need a little help tracking down your Android dependencies.

This is most easily accomplished by installing all Android dependencies in the same folder. You can place this folder wherever you like on your machine. In this build guide and across our CMakeLists files this folder is referred to as `ANDROID_LIB_DIR`. You can set `ANDROID_LIB_DIR` in your environment or by passing when you run CMake.

#### Qt

Install Qt 5.5.1 for Android for your host environment from the [Qt downloads page](http://www.qt.io/download/). Install Qt to ``$ANDROID_LIB_DIR/Qt``. This is required so that our root CMakeLists file can help CMake find your Android Qt installation.

The component required for the Android build is the `Android armv7` component.

If you would like to install Qt to a different location, or attempt to build with a different Qt version, you can pass `ANDROID_QT_CMAKE_PREFIX_PATH` to CMake. Point to the `cmake` folder inside `$VERSION_NUMBER/android_armv7/lib`. Otherwise, our root CMakeLists will set it to `$ANDROID_LIB_DIR/Qt/5.5/android_armv7/lib/cmake`.

#### OpenSSL

Cross-compilation of OpenSSL has been tested from an OS X machine running 10.10 compiling OpenSSL 1.0.2. It is likely that the steps below will work for other OpenSSL versions than 1.0.2.

The original instructions to compile OpenSSL for Android from your host environment can be found [here](http://wiki.openssl.org/index.php/Android). We required some tweaks to get OpenSSL to successfully compile, those tweaks are explained below.

Download the [OpenSSL source](https://www.openssl.org/source/) and extract the tarball inside your `ANDROID_LIB_DIR`. Rename the extracted folder to `openssl`.

You will need the [setenv-android.sh script](http://wiki.openssl.org/index.php/File:Setenv-android.sh) from the OpenSSL wiki.

You must change three values at the top of the `setenv-android.sh` script - `_ANDROID_NDK`, `_ANDROID_EABI` and `_ANDROID_API`.
`_ANDROID_NDK` should be `android-ndk-r10`, `_ANDROID_EABI` should be `arm-linux-androidebi-4.9` and `_ANDROID_API` should be `19`.

First, make sure `ANDROID_NDK_ROOT` is set in your env. This should be the path to the root of your Android NDK install. `setenv-android.sh` needs `ANDROID_NDK_ROOT` to set the environment variables required for building OpenSSL.

Source the `setenv-android.sh` script so it can set environment variables that OpenSSL will use while compiling. If you use zsh as your shell you may need to modify the `setenv-android.sh` for it to set the correct variables in your env.

```
export ANDROID_NDK_ROOT=YOUR_NDK_ROOT
source setenv-android.sh
```

Then, from the OpenSSL directory, run the following commands.

```
perl -pi -e 's/install: all install_docs install_sw/install: install_docs install_sw/g' Makefile.org
./config shared -no-ssl2 -no-ssl3 -no-comp -no-hw -no-engine --openssldir=/usr/local/ssl/$ANDROID_API
make depend
make all
```

This should generate libcrypto and libssl in the root of the OpenSSL directory. YOU MUST remove the `libssl.so` and `libcrypto.so` files that are generated. They are symlinks to `libssl.so.VER` and `libcrypto.so.VER` which Android does not know how to handle. By removing `libssl.so` and `libcrypto.so` the FindOpenSSL module will find the static libs and use those instead.

If you have been building other components it is possible that the OpenSSL compile will fail based on the values other cross-compilations (tbb, bullet) have set. Ensure that you are in a new terminal window to avoid compilation errors from previously set environment variables.

#### Oculus Mobile SDK

The Oculus Mobile SDK is optional, for Gear VR support. It is not required to compile gvr-interface.

Download the [Oculus Mobile SDK](https://developer.oculus.com/downloads/#sdk=mobile) and extract the archive inside your `ANDROID_LIB_DIR` folder. Rename the extracted folder to `libovr`.

From the VRLib directory, use ndk-build to build VrLib.

```
cd VRLib
ndk-build
```

This will create the liboculus.a archive that our FindLibOVR module will look for when cmake is run.

##### Hybrid testing

Currently the 'vr_dual' mode that would allow us to run a hybrid app has limited support in the Oculus Mobile SDK. The best way to have an application we can launch without having to connect to the GearVR is to put the Gear VR Service into developer mode. This stops Oculus Home from taking over the device when it is plugged into the Gear VR headset, and allows the application to be launched from the Applications page.

To put the Gear VR Service into developer mode you need an application with an Oculus Signature File on your device. Generate an Oculus Signature File for your device on the [Oculus osig tool page](https://developer.oculus.com/tools/osig/). Place this file in the gvr-interface/assets directory. Cmake will automatically copy it into your apk in the right place when you execute `make gvr-interface-apk`.

Once the application is on your device, go to `Settings->Application Manager->Gear VR Service->Manage Storage`. Tap on `VR Service Version` six times. It will scan your device to verify that you have an osig file in an application on your device, and then it will let you enable Developer mode.

### CMake

We use CMake to generate the makefiles that compile and deploy the Android APKs to your device. In order to create Makefiles for the Android targets, CMake requires that some environment variables are set, and that other variables are passed to it when it is run.

The following must be set in your environment:

* ANDROID_NDK - the root of your Android NDK install
* ANDROID_HOME - the root of your Android SDK install
* ANDROID_LIB_DIR - the directory containing cross-compiled versions of dependencies

The following must be passed to CMake when it is run:

* USE_ANDROID_TOOLCHAIN - set to true to build for Android
