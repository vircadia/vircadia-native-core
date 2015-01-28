Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only Android specific instructions are found in this file.

###Android Dependencies

You will need the following tools to build our Android targets.

* [Android NDK](https://developer.android.com/tools/sdk/ndk/index.html) = r10c
* [Android SDK](http://developer.android.com/sdk/installing/index.html) ~> 24.0.2
  * Be sure to install SDK Platform for API Level 19

You will also need to cross-compile the dependencies required for all platforms for Android, and help CMake find these compiled libraries on your machine.

####Optional Components

* [Oculus Mobile SDK](https://developer.oculus.com/downloads/#sdk=mobile) ~> 0.4.2

####ANDROID_LIB_DIR

Since you won't be installing Android dependencies to system paths on your development machine, CMake will need a little help tracking down your Android dependencies. 

This is most easily accomplished by installing all Android dependencies in the same folder. You can place this folder wherever you like on your machine. In this build guide and across our CMakeLists files this folder is referred to as `ANDROID_LIB_DIR`. You can set `ANDROID_LIB_DIR` in your environment or by passing when you run CMake.

####Qt

Install Qt 5.3 for Android for your host environment from the [Qt downloads page](http://www.qt.io/download/). Install Qt to ``$ANDROID_LIB_DIR/Qt``. This is required so that our root CMakeLists file can help CMake find your Android Qt installation.

If you would like to install Qt to a different location, or attempt to build with a different Qt version, you can pass `ANDROID_QT_CMAKE_PREFIX_PATH` to CMake. Point to the `cmake` folder inside `$VERSION_NUMBER/android_armv7/lib`. Otherwise, our root CMakeLists will set it to `$ANDROID_LIB_DIR/Qt/5.3/android_armv7/lib/cmake`.

####OpenSSL

Cross-compilation of OpenSSL has only been tested from an OS X machine running 10.10 compiling OpenSSL 1.0.1i. It is likely that the steps below will work for other OpenSSL versions than 1.0.1i.

The original instructions to compile OpenSSL for Android from your host environment can be found [here](http://wiki.openssl.org/index.php/Android). We required some tweaks to get OpenSSL to successfully compile, those tweaks are explained below.

Download the [OpenSSL source](https://www.openssl.org/source/) and extract the tarball inside your `ANDROID_LIB_DIR`. Rename the extracted folder to `openssl`.

You will need the [setenv-android.sh script](http://wiki.openssl.org/index.php/File:Setenv-android.sh) from the OpenSSL wiki. 

You must change two values at the top of the `setenv-android.sh` script - `_ANDROID_NDK` and `_ANDROID_EABI`.
`_ANDROID_NDK` should be `android-ndk-r10` and `_ANDROID_EABI` should be `arm-linux-androidebi-4.9`.

First, make sure `ANDROID_NDK_ROOT` is set in your env. This should be the path to the root of your Android NDK install. `setenv-android.sh` needs `ANDROID_NDK_ROOT` to set the environment variables required for building OpenSSL.

Source the `setenv-android.sh` script so it can set environment variables that OpenSSL will use while compiling. If you use zsh as your shell you may need to modify the `setenv-android.sh` for it to set the correct variables in your env.

We have had issues with `setenv-android.sh` not helping the system use the Android archive tool during compilation. You may also need to set `AR` to point to the `ar` from your NDK AFTER running ./setenv-android.sh. 

Note that your path to `arm-linux-androideabi-ar` will probably not be the same as the one below if you are not on OS X or are using a different EABI.

```
export ANDROID_NDK_ROOT=YOUR_NDK_ROOT
source setenv-android.sh
export AR=$ANDROID_NDK_ROOT/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-ar
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

####Intel Threading Building Blocks

Download the [Intel Threading Building Blocks source](https://www.threadingbuildingblocks.org/download) and extract the tarball inside your `ANDROID_LIB_DIR`. Rename the extracted folder to `tbb`.

NOTE: BEFORE YOU ATTEMPT TO CROSS-COMPILE TBB, DISCONNECT ANY DEVICES ADB WOULD DETECT. The tbb build process asks adb for a couple of strings, and if a device is plugged in extra characters get added that will cause ndk-build to fail with an error.

From the tbb directory, execute the following commands. First, we build TBB using `ndk-build`. Then, the compiled libs are copied to a lib folder in the root of tbb directory.

```
cd jni
ndk-build target=android tbb tbbmalloc arch=arm
cd ../
mkdir lib
cp `find . -name "*.so"` lib/
```

####Soxr

Download the [Soxr source](http://sourceforge.net/projects/soxr/) and extract the tarball inside your `ANDROID_LIB_DIR`. Rename the extracted folder to `soxr`.

From the soxr directory, use cmake, along with the `android.toolchain.cmake` file (included in this repository under cmake/android) to cross-compile soxr for Android.

The full set of commands to build soxr for Android is shown below

```
cmake -DCMAKE_TOOLCHAIN_FILE=$FULL_PATH_TO_TOOLCHAIN -DHAVE_WORDS_BIGENDIAN_EXITCODE=1 -DBUILD_TESTS=0 -DCMAKE_INSTALL_PREFIX=.
make
make install
```

This will create the `lib` and `include` folders inside `ANDROID_LIB_DIR/soxr` that FindSoxr will look for.

####Oculus Mobile SDK

The Oculus Mobile SDK is optional, for Gear VR support. It is not required to compile gvr-interface.

Download the [Oculus Mobile SDK](https://developer.oculus.com/downloads/#sdk=mobile) and extract the archive inside your `ANDROID_LIB_DIR` folder. Rename the extracted folder to `libovr`.

From the VrLib directory, use ndk-build to build VrLib. This will create the liboculus.a archive that our FindLibOVR module will look for when cmake is run.

#####Hybrid testing

Currently the 'vr_dual' mode that would allow us to run a hybrid app has limited support in the Oculus Mobile SDK. The best way to have an application we can launch without having to connect to the GearVR is to put the Gear VR Service into developer mode. This stops Oculus Home from taking over the device when it is plugged into the Gear VR headset, and allows the application to be launched from the Applications page.

To put the Gear VR Service into developer mode you need an application with an Oculus Signature File on your device. Generate an Oculus Signature File for your device on the [Oculus osig tool page](https://developer.oculus.com/tools/osig/). Place this file in the gvr-interface/assets directory. Cmake will automatically copy it into your apk in the right place when you execute `make gvr-interface-apk`.

Once the application is on your device, go to `Settings->Application Manager->Gear VR Service->Manage Storage`. Tap on `VR Service Version` six times. It will scan your device to verify that you have an osig file in an application on your device, and then it will let you enable Developer mode.

####GLM

Since GLM is a header only library, assuming it is installed at a system path or a path where our FindGLM module will find it you do not need to do anything specific for the Android build.

###CMake

We use CMake to generate the makefiles that compile and deploy the Android APKs to your device. In order to create Makefiles for the Android targets, CMake requires that some environment variables are set, and that other variables are passed to it when it is run.

The following must be set in your environment:

* ANDROID_NDK - the root of your Android NDK install
* ANDROID_HOME - the root of your Android SDK install
* ANDROID_LIB_DIR - the directory containing cross-compiled versions of dependencies

The following must be passed to CMake when it is run:

* CMAKE_TOOLCHAIN_FILE - full path to the android.toolchain.cmake file that is included in this repository (/cmake/android/android.toolchain.cmake)
* ANDROID_NATIVE_API_LEVEL - the API level you want to use (this should be 19 for GearVR)
