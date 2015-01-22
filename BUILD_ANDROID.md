Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only Android specific instructions are found in this file.

###Android Dependencies

You will need the following tools to build our Android targets.

* [Android NDK](https://developer.android.com/tools/sdk/ndk/index.html) = r10c
* [Android SDK](http://developer.android.com/sdk/installing/index.html) ~> 24.0.2
  * Be sure to install SDK Platform for API Level 19

You will also need to cross-compile the dependencies required for all platforms for Android, and help CMake find these compiled libraries on your machine.

####ANDROID_LIB_DIR

Since you won't be installing Android dependencies to system paths on your development machine, CMake will need a little help tracking down your Android dependencies. 

This is most easily accomplished by installing all Android dependencies in the same folder. You can place this folder wherever you like on your machine. In this build guide and across our CMakeLists files this folder is referred to as `ANDROID_LIB_DIR`. You can set `ANDROID_LIB_DIR` in your environment or by passing when you run CMake.

####Qt

Install Qt 5.3 for Android for your host environment from the [Qt downloads page](http://www.qt.io/download/). Install Qt to ``$ANDROID_LIB_DIR/Qt``. This is required so that our root CMakeLists file can help CMake find your Android Qt installation.

If you would like to install Qt to a different location, or attempt to build with a different Qt version, you can pass `ANDROID_QT_CMAKE_PREFIX_PATH` to CMake. Point to the `cmake` folder inside `$VERSION_NUMBER/android_armv7/lib`. Otherwise, our root CMakeLists will set it to `$ANDROID_LIB_DIR/Qt/5.3/android_armv7/lib/cmake`.

####OpenSSL

Cross-compilation of OpenSSL has only been tested from an OS X machine running 10.10 compiling OpenSSL 1.0.1j. It is likely that the steps below will work for other OpenSSL versions than 1.0.1j.

The full instructions to compile OpenSSL for Android from your host environment can be found [here](http://wiki.openssl.org/index.php/Android).

Download the [OpenSSL source](https://www.openssl.org/source/) and extract the tarball inside your `ANDROID_LIB_DIR`. Rename the extracted folder to `openssl`.

You will need the [setenv-android.sh script](http://wiki.openssl.org/index.php/File:Setenv-android.sh) from the OpenSSL wiki. 

First, make sure `ANDROID_NDK_ROOT` is set in your env. This should be the path to the root of your Android NDK install. If you've configured your machine to build the Android client using the instructions below, you can set it to the value of $ANDROID_NDK. `setenv-android.sh` needs `ANDROID_NDK_ROOT` to set the environment variables required for building OpenSSL.

Execute the `setenv-android.sh` script so it can set environment variables that OpenSSL will use while compiling.

We have had issues with `setenv-android.sh` not helping the system use the Android archive tool during compilation. You may also need to set `AR` to point to the `ar` from your NDK AFTER running ./setenv-android.sh. 

Note that your path to `arm-linux-androideabi-ar` will probably not be the same as the one below if you are not on OS X or are using a different EABI.

```
export ANDROID_NDK_ROOT=YOUR_NDK_ROOT
./setenv-android.sh
export AR=$ANDROID_NDK_ROOT_/toolchains/arm-linux-androideabi-4.6/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-ar
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

From the tbb directory, execute the following commands. This will set the compiler and archive tool to the correct ones from the NDK install and then build TBB using `ndk-build`. Then, the compiled libs are copied to a lib folder in the root of tbb directory.

Note that you will need to replace the value of $HOST below with whatever is appropriate for your host OS. On OS X, for example, the full exported value for CC is `$ANDROID_NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-gcc`.

```
export CC=$ANDROID_NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/$HOST/bin/arm-linux-androideabi-gcc
export AR=$ANDROID_NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/$HOST/bin/arm-linux-androideabi-ar
cd jni
ndk-build target=android tbb tbbmalloc arch=arm
cd ../
mkdir lib
cp -rf build/linux_arm_*/**/*.so lib/
```

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
