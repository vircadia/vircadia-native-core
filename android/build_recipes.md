Different libraries require different mechanism for building.  Some are easiest with the standalone toolchain. Some are easiest with the ndk-build tool.  Some can rely on CMake to do the right thing.

## Setup

### Android build environment

You need the Android NDK and SDK.  The easiest way to get these is to download Android Studio for your platform and use the SDK manager in Studio to install the items.  In particular you will need Android API levels 24 and 26, as well as the NDK, CMake, and LLDB.  You should be using NDK version 16 or higher.

The Studio installation can install the SDK wherver you like.  This guide assumes you install it to `$HOME/Android/SDK`.  It will install the NDK inside the SDK in a folder named `ndk-bundle` but for convenience we will assume a symlink from `$HOME/Android/NDK` to the actual NDK folder exists
 
Additionally, some of the tools require a standalone toolchain in order to build.  From the NDK build/tools directory you can execute the following command

`./make-standalone-toolchain.sh --arch=arm64 --platform=android-24 --install-dir=$HOME/Android/arm64_toolchain`

This will create the toolchain and install it in `$HOME/Android/arm64_toolchain`

When doing a build that relies on the toolchain you can execute the following commands to enable it

```
target_host=aarch64-linux-android
export PATH=$PATH:$HOME/Android/arm64_toolchain/bin
export AR=$target_host-ar
export AS=$target_host-as
export CC=$target_host-gcc
export CXX=$target_host-g++
export LD=$target_host-ld
export STRIP=$target_host-strip
export CFLAGS="-fPIE -fPIC"
export LDFLAGS="-pie"
```

  

## Qt

### Windows host

* Install the Android SDK
* Install the Android NDK	
* Install Git for Windows
* Install Strawberry Perl
* Install Java 8 (Do NOT use Java 9, it will fail)
* Install Python 3.6 for Windows
* Open a Git Bash command prompt
* Ensure the following commands are visible in the path with `which <command>`
   * gcc
   * javac
   * python
   * gmake
* If any of them fail, fix your path and restart the bash prompt
* Fetch the pre-built OpenSSL binaries for Android/iOS from here:  https://github.com/leenjewel/openssl_for_ios_and_android/releases
  * Grab the latest release of the 1.0.2 series
  * Open the archive and extract the `/android/openssl-arm64-v8a` folder 
   
### All platforms

* Download the Qt sources 
  * `git clone git://code.qt.io/qt/qt5.git`
  * `cd qt5`
  * `perl init-repository`
  * `git checkout v5.9.3`
  * `git submodule update --recursive`
  * `cd ..`
* Create a build directory with the command `mkdir qt5build`
* Configure the Qt5 build with the command `../qt5/configure -opensource -confirm-license -xplatform android-clang --disable-rpath -nomake tests -nomake examples -skip qttranslations -skip qtserialport -skip qt3d -skip qtwebengine -skip qtlocation -skip qtwayland -skip qtsensors -skip qtgamepad -skip qtgamepad -skip qtspeech -skip qtcharts -skip qtx11extras -skip qtmacextras -skip qtvirtualkeyboard -skip qtpurchasing -skip qtdatavis3d  -android-toolchain-version 4.9 -android-ndk $HOME/Android/NDK -android-arch arm64-v8a -no-warnings-are-errors -android-ndk-platform android-24 -v -android-ndk-host windows-x86_64 -platform win32-g++ -prefix C:/qt5build_debug -android-sdk $HOME/Android/SDK -c++std c++14 -openssl-linked -L<PATH_TO_SSL>/lib -I<PATH_TO_SSL>/include`
* Some of those entries must be customized depending on platform.
  * `-platform win32-g++` 
  * `-android-ndk-host windows-x86_64`
  * `-prefix C:/qt5build_debug` 


   
## TBB

Use the ndk-build tool

ndk-build tbb tbbmalloc target=android arch=ia32 tbb_os=windows ndk_version=16

## OpenSSL

Use a standalone toolchain

* Grab the latest 1.1.0x series source from https://github.com/openssl/openssl/releases
* Follow the NDK guidelines for building a standalone toolchain for aarch64
* Use the following script to configure and build OpenSSL 
* Enable the standalone toolchain with the export commands described above
* Configure SSL with the command `./Configure android64-aarch64 no-asm no-ssl2 no-ssl3 no-comp no-hw no-engine --prefix=$HOME/Android/openssl_1.1.0g` 
* Build and install SSL with the command `make depend && make && make install`

 


   
