Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only Android specific instructions are found in this file.

###Android Dependencies

There are no Android specific dependencies to build hifi. However, you will need to compile the dependencies required for all platforms for Android, and help CMake find these compiled libraries on your machine.

####ANDROID_LIB_DIR

Since you won't be installing Android dependencies to system paths on your development machine, CMake will need a little help tracking down your Android dependencies. 

This is most easily accomplished by installing all Android dependencies in the same folder. You can place this folder wherever you like on your machine. In this build guide and across our CMakeLists files this folder is referred to as `ANDROID_LIB_DIR`. You can set `ANDROID_LIB_DIR` in your environment or by passing when you run CMake.

####Qt

While the current build guide states we are using Qt 5.3.0, the Android build uses 5.4.0. This is because Qt Android support is newer and more likely to be receiving important bug fixes between versions.

Install Qt 5.4.0 for Android for your host environment from the [Qt downloads page](http://www.qt.io/download/). Install Qt to ``$ANDROID_LIB_DIR/Qt``. This is required so that our root CMakeLists file can help CMake find your Android Qt installation.

If you would like to install Qt to a different location, or attempt to build with a different Qt version, you can pass `ANDROID_QT_CMAKE_PREFIX_PATH` to CMake. Point to the `cmake` folder inside `$VERSION_NUMBER/android_armv7/lib`. Otherwise, our root CMakeLists will set it to `$ANDROID_LIB_DIR/Qt/5.4/android_armv7/lib/cmake`.

####OpenSSL

Cross-compilation of OpenSSL has only been tested from an OS X machine running 10.10 compiling OpenSSL 1.0.1j. It is likely that the steps below will work for other OpenSSL versions than 1.0.1j.

Download the (OpenSSL source)[https://www.openssl.org/source/] and extract the tarball inside your `ANDROID_LIB_DIR`. Rename the extracted folder to `openssl`.

You will need the [setenv-android.sh script](http://wiki.openssl.org/index.php/File:Setenv-android.sh) from the OpenSSL wiki. 

First, make sure `ANDROID_NDK_ROOT` is set in your env. `setenv-android.sh` needs `ANDROID_NDK_ROOT`.

Execute the `setenv-android.sh` script so it can set environment variables that OpenSSL will use while compiling.

Then, from the OpenSSL directory, run the following commands.

```
perl -pi -e 's/install: all install_docs install_sw/install: install_docs install_sw/g' Makefile.org 
./config shared -no-ssl2 -no-ssl3 -no-comp -no-hw -no-engine --openssldir=/usr/local/ssl/$ANDROID_API 
make depend
make all
```

This should generate libcrypto and libssl in the root of the OpenSSL directory.

If you have been building other components it is possible that the OpenSSL compile will fail based on the values other cross-compilations (tbb, bullet) have set. Ensure that you are in a new terminal window to avoid compilation errors from previously set environment variables.

####Intel Threading Building Blocks

Download the (Intel Threading Building Blocks source)[https://www.threadingbuildingblocks.org/download] and extract the tarball inside your `ANDROID_LIB_DIR`. Rename the extracted folder to `tbb`.

From the tbb directory, execute the following commands. This will set the compiler and archive tool to the correct ones from the NDK install and then build TBB using `ndk-build`.

```
export CC=$ANDROID_NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-gcc
export AR=$ANDROID_NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-ar
cd jni
ndk-build target=android tbb tbbmalloc arch=arm
cd ../
cp -rf build/linux_arm_*/**/*.so lib/
``` 

####GLM

Since GLM is a header only library, assuming it is installed at a system path or a path where our FindGLM module will find it you do not need to do anything specific for the Android build.
