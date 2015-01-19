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

####GLM

Since GLM is a header only library, assuming it is installed at a system path or a path where our FindGLM module will find it you do not need to do anything specific for the Android build.
