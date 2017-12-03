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
* Download the Qt sources 
  * `git clone git://code.qt.io/qt/qt5.git`
  * `cd qt5`
  * `perl init-repository`
  * `git checkout v5.9.3`
  * `git submodule update --recursive`
  * `cd ..`
* Create a build directory with the command `mkdir qt5build`
* Configure the Qt5 build with the command `../qt5/configure -xplatform android-clang -android-ndk-host windows-x86_64 -confirm-license -opensource  --disable-rpath -nomake tests -nomake examples -skip qttranslations -skip qtserialport -skip qt3d -skip qtwebengine -skip qtlocation -skip qtwayland -skip qtsensors -skip qtgamepad -skip qtgamepad -skip qtspeech -skip qtcharts -skip qtx11extras -skip qtmacextras -skip qtvirtualkeyboard -skip qtpurchasing -skip qtdatavis3d  -android-ndk C:/Android/NDK -android-toolchain-version 4.9 -android-arch arm64-v8a -no-warnings-are-errors -android-ndk-platform android-24 -v -platform win32-g++ -prefix C:/qt5build_debug -android-sdk C:/Android/SDK `
   