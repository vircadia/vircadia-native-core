### Android build workarounds

These are various hacks and workarounds I used to build the client library for Android. This is Linux specific and not a proper build configuration, so other components might not build or function fully with these changes.

### Pre requisites

#### Android SDK and NDK, OpenJDK
Install OpenJDK 8, should be available on most pretty much all linux distros.
```
sudo apt install openjdk-8-jdk
```
Download android command line tools, currently available [here](https://developer.android.com/studio#downloads).

Using the `tools/bin/sdkmanager` install platform, build tools and ndk.
```
sdkmanager "platforms;android-28" "build-tools;28.0.2" "ndk;21.3.6528147"
```

#### Qt

Download Qt 5.15.x source package currently available [here](https://www.qt.io/offline-installers).<br />
Export JDK paths:
```
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export PATH=$JAVA_HOME/bin:$PATH
```
In the Qt source root create a build directory:
```
mkdir build
cd build
```
Configure using the [android/qt_configure_android.sh](android/qt_configure_android.sh) script, adjusting the `-android-ndk`, `-android-sdk` and `--prefix` paths according to your setup.<br />
Build and install:
```
make -j4
make install -j4
```
Qt is built for all architectures by default so you don't need to worry about that here.

#### TBB
Clone oneTBB repo from github (mater is on 9d2a3477ce276d437bf34b1582781e5b11f9b37a at the time of writing this)
```
git clone https://github.com/oneapi-src/oneTBB
```
Apply [android/tbb_remove_emit.patch](android/tbb_remove_emit.patch)
```
git am <path_to_vircadia>/libraries/vircadia-client/android/tbb_remove_emit.patch
```
Create a build directory for specific architecture
```
mkdir build_armeabi-v7a
cd build_armeabi-v7a
```
Configure using [android/tbb_configure_android.sh](android/tbb_configure_android.sh), adjusting `-DANDROID_ABI` (armeabi-v7a, arm64-v8a, x86, x86_64), `-DCMAKE_TOOLCHAIN_FILE` (should be present in NDK) according to your setup and desired architecture (for multiple architectures will need to repeat the process in a different build folder). <br />
Build and install.
```
make -j4
make install -j4
```
The library will be installed in `tbb_install` directory in the build directory (in this case `build_armeabi-v7a/tbb_install`).

#### OpenSSL
Download [OpenSSL 1.1.1n source archive](https://github.com/openssl/openssl/releases/tag/OpenSSL_1_1_1n).<br />
Export paths to android NDK toolchains for all desired architectures.
```
export ANDROID_NDK_HOME=/home/namark/dev/android/ndk/21.3.6528147
export PATH=$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin:$ANDROID_NDK_HOME/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin:$ANDROID_NDK_HOME/toolchains/aarch64-linux-android-4.9/prebuilt/linux-x86_64/bin:$PATH
```
Apply [android/openssl_fix_includes.diff](android/openssl_fix_includes.diff) in the root of the source archive.
```
patch -p0 < <path_to_vircadia>/libraries/vircadia-client/android/openssl_fix_includes.diff
```
Create a build directory for specific architecture
```
mkdir build_armeabi-v7a
cd build_armeabi-v7a
```
Configure using [android/openssl_configure_android.sh](android/openssl_configure_android.sh), changing `android-arm` to desired architecture (`android-arm`, `android-arm64`, `android-x86`, `android-x86_64`) and "--prefix" parameter to full path of desired installation directory.<br />
Build and install:
```
make -j4
make install -j4
```

### Project setup wreckage
Make sure to navigate to the vircadia project root.<br />
Apply wreckage.patch using git
```
git am libraries/vircadia-client/android/wreckage.patch
```

Hardcode `QT_CMAKE_PTEFIX_PATH` for android in CMakeLists.txt in project root to the path of qt installation.<br />
Hardcode `OPENSSL_INSTALL_DIR` for android in `cmake/macros/TargetOpenSSL.cmake` to the path of OpenSSl installation for desired architecture.<br />
Create a build directory for desired architecture.
```
mkdir build_armeabi-v7a
cd build_armeabi-v7a
```
Configure using the following cmake command, adjusting `-DCMAKE_TOOLCHAIN_FILE`, `-DANDROID_ABI` as in TBB instructions above, and -DTBB_DIR according to your TBB installation for specific architecture.
```
cmake .. -DCMAKE_SYSTEM_NAME=Android -DCMAKE_TOOLCHAIN_FILE=/home/namark/dev/android/ndk-bundle/build/cmake/android.toolchain.cmake -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=android-28 -DHIFI_ANDROID=ON -DTBB_DIR=/home/namark/dev/oneTBB/build/tbb_install/lib/cmake/TBB -DHIFI_ANDROID_APP=dummy -DVIRCADIA_OPTIMIZE=OFF -DUSE_IPFS_EXTERNAL_BUILD_ASSETS=OFF -DVIRCADIA_CPU_ARCHITECTURE="" -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX:PATH=vircadia-client-package
```
Note: First time cmake is ran it may produce an error about the compiler:
```
  The C++ compiler

    "/home/namark/dev/android/ndk-bundle/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++"

  is not able to compile a simple test program.
```
Running the second time fixes it somehow.


Finally build and install the library (will be installed in `libraries/vircadia-client/vircadia-client-package` directory in the build directory).
```
cmake --build . --target vircadia
cd libraries/vircadia-client
cmake --build . --target install/strip
```


None of the shared binaries get stripped for some reason, so I had to do it manually using appropriate `strip` binaries from ndk. For example:
```
~/dev/android/ndk/21.3.6528147/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-strip vircadia-client-package/lib/libvircadia-client_armeabi-v7a.so
```
