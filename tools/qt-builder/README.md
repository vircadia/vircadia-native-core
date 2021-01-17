# General
This document describes the process to build Qt 5.15.2.

Reference: https://doc.qt.io/qt-5/build-sources.html

Note that there are patches.
* *win-qtwebengine.diff* fixes building with Visual Studio 2019 16.8.0.  See https://bugreports.qt.io/browse/QTBUG-88625.


## Requirements

### Windows
1.  Visual Studio 2019  
If you don’t have Community or Professional edition of Visual Studio 2019, download [Visual Studio Community 2019](https://www.visualstudio.com/downloads/).  
Install with C++ support.
1.  python 2.7.x >= 2.7.16  
Download MSI installer from https://www.python.org/ and add directory of the python executable to PATH.
NOTE:  REMOVE python 3 from PATH. (Regular Vircadia builds use python 3, however these will still work because 
the make files automatically handle this.)
Verify by running `python.exe --version`
1.  git >= 1.6  
Download from https://git-scm.com/download/win  
Verify by running `git --version`  
1.  perl >= 5.14  
Install from Strawberry Perl - http://strawberryperl.com/ - 5.28.1.1 64 bit to C:\Strawberry\  
Verify by running `perl --version`  
1.  flex and bison  
These are provided as part of the Qt repository in the gnuwin32/bin folder, which is prepended to the command prompt PATH when 
`tq5vars.bat` is run (see below).  
Alternatively, you can download it from https://sourceforge.net/projects/winflexbison/files/latest/download and...  
: Uncompress in C:\flex_bison  
: Rename win-bison.exe to bison.exe and win-flex.exe to flex.exe  
: Add C:\flex_bison to PATH  
Verify by running `flex --version`  
Verify by running `bison --version`  
1.  gperf  
This is provided as part of the Qt repository in the gnuwin32/bin folder, which is prepended to the command prompt PATH when 
`tq5vars.bat` is run (see below).  
alternatively, you can install from http://gnuwin32.sourceforge.net/downlinks/gperf.php and...  
: Add C:\Program Files (x86)\GnuWin32\bin to PATH  
Verify by running `gperf --version`  
1.  7-zip  
Download from https://www.7-zip.org/download.html and install.  
1.  Bash shell  
Open Control Panel and select *Programs and Features*  
Select *Turn Windows features on or off*  
Check *Windows Subsystem for Linux*  
Click *Restart now*  
Download from the Microsoft Store - Search for *bash* and choose the latest Ubuntu version  
[First run will take a few minutes]  
Enter a user name - all small letters (this is used for *sudo* commands)  
Choose a password
1.  Jom  
jom is a clone of nmake to support the execution of multiple independent commands in parallel.  
https://wiki.qt.io/Jom  
Install in C:\Jom and add directory to PATH.

### Linux
**TODO: Update this section for Qt 5.15.2**  
Tested on Ubuntu 16.04 and 18.04.  
**16.04 NEEDED FOR JENKINS~~  **
1.  qt5 requirements  
edit /etc/apt/sources.list (edit as root)  
replace all *# deb-src* with *deb-src* (in vi `1,$s/# deb-src/deb-src/`)  
`sudo apt-get update -y`  
`sudo apt-get upgrade -y`  
`sudo apt-get build-dep qt5-default -y`  
1.  git >= 1.6  
Check if needed `git --version`  
`sudo apt-get install git -y`  
Verify again  
1.  python  
Check if needed `python --version` - should return python 2.7.x  
`sudo apt-get install python -y` (not python 3!)  
Verify again  
1.  gperf  
Check if needed `gperf --version`  
`sudo apt-get install gperf -y`  
Verify again  
1.  bison and flex  
Check if needed `flex --version`  and `bison --version`  
`sudo apt-get install flex bison -y`  
Verify again  
1.  pkg-config (needed for qtwebengine)  
Check if needed `pkg-config --version`  
`sudo apt-get install pkg-config -y`  
Verify again  
1.  OpenGL  
Verify (first install mesa-utils - `sudo apt install mesa-utils -y`) by `glxinfo | grep "OpenGL version"`  
`sudo apt-get install libgl1-mesa-dev -y`  
`sudo ln -s /usr/lib/x86_64-linux-gnu/libGL.so.346.35 /usr/lib/x86_64-linux-gnu/libGL.so.1.2.0`  
Verify again  
1.  make  
Check if needed `make --version`  
`sudo apt-get install make -y`  
Verify again  
1.  g++  
Check if needed  
 `g++ --version`  
`sudo apt-get install g++ -y`  
Verify again  
1.  dbus-1 (needed for qtwebengine)  
`sudo apt-get install libdbus-glib-1-dev -y`  
1.  nss (needed for qtwebengine)  
`sudo apt-get install libnss3-dev -y`  

### Mac  
1.  git >= 1.6  
Check if needed `git --version`  
Install from https://git-scm.com/download/mac  
Verify again  
1.  pkg-config  
brew fontconfig dbus-glib pkg-config  
1.  dbus-1  
brew install dbus-glib  


## Build Process


### General
Qt is cloned to a qt5 folder.  
The build is performed in a qt5/qt5-build folder.  
Build products are installed to the qt5/qt5-install folder.  
Before running configure, make sure that the qt5-build folder is empty.  


### Windows
Make sure that the directory you are using to build Qt is not deeply nested.  It is quite possible to run into the windows MAX_PATH limit when building chromium.  For example: `c:\msys64\home\ajt\code\hifi\tools\qt-builder\qt5-build` is too long.  `c:\q\qt5-build\` is a better choice.  

#### Preparing OpenSSL

Do one of the following to provide OpenSSL for building against:  
a. If you have installed a local copy of Qt 5.15.2 and have built Vircadia against that, including OpenSSL, find the 
**HIFI_VCPKG_BASE** subdirectory used in your build and make an environment variable **HIFI_VCPKG_BASE_DIR** point to the 
*installed\x64-windows* folder. 
a. Follow https://github.com/vircadia/vcpkg to install *vcpkg* and then *openssl*.
Then make an environment variable **HIFI_VCPKG_BASE_DIR** point to the *packages\openssl-windows_x64-windows* vcpkg folder.

#### Preparing source files
Get the source:  
`git clone --recursive https://code.qt.io/qt/qt5.git -b 5.15.2 --single-branch`

Apply patches:  
*  Copy the **patches** folder to qt5
*  Apply the patches to Qt  
`cd qt5`  
`git apply --ignore-space-change --ignore-whitespace patches/win-qtwebengine.diff`  
`cd ..` 

Set up configuration batch file:
*  Copy the **qt5vars.bat** file to qt5.  
*  Edit the **qt5vars.bat** file per the instructions in it.

#### Configuring  

Do the following in a Visual Studio 2019 Command Prompt window.

Set up the build directories:  
`cd qt5`  
`mkdir qt5-build`  
`mkdir qt5-install`  
`cd qt5-build`

Set up the environment variables:  
`..\qt5vars`  

Note: If you need to rebuild, wipe the *qt5-build* and *qt5-install* directories and re-run `qt5vars`.

Configure the Qt build:

`..\configure -force-debug-info -opensource -confirm-license -opengl desktop -platform win32-msvc -openssl-linked -I %HIFI_VCPKG_BASE_DIR%\include -L %HIFI_VCPKG_BASE_DIR%\lib OPENSSL_LIBS="-llibcrypto -llibssl" -nomake examples -nomake tests -skip qttranslations -skip qtserialport -skip qt3d -skip qtlocation -skip qtwayland -skip qtsensors -skip qtgamepad -skip qtcharts -skip qtx11extras -skip qtmacextras -skip qtvirtualkeyboard -skip qtpurchasing -skip qtdatavis3d -no-warnings-are-errors -no-pch -prefix ..\qt5-install`

Copy the files *zlib1.dll*  and *zlib.pdb* from the %HIFI_VCPKG_BASE_DIR%\bin directory to *qt5-build\qtbase\bin* (which is on 
the PATH used by the `jom` command).  
FIXME: Why aren't these zlib files automatically provided / found for use by rcc.exe?  

#### Make

Build Qt:  
`jom`  
`jom install`  

#### Fixing

The *.prl* files have an absolute path that needs to be removed (see http://www.linuxfromscratch.org/blfs/view/stable-systemd/x/qtwebengine.html)  
1.  `cd` to the *qt5-install* folder. 
1.  Open a bash terminal. (Run `bash` in command prompt.)  
1.  Run the following command:  
`find . -name \*.prl -exec sed -i -e '/^QMAKE_PRL_BUILD_DIR/d' {} \;`  

Add a *qt.conf* file.  
1. Copy the file *qt5-build\qtbase\bin\qt.conf* to *qt5-install\bin*.  
1. Edit the *qt.conf* file: replace all absolute URLs with relative URLs.

Copy the files *zlib1.dll*  and *zlib.pdb* from *qt5-build\qtbase\bin* to *qt5-install\bin*.  
FIXME: Why aren't these zlib files automatically included?  


#### Uploading

Create a Gzip tar file called qt5-install-5.15.2-windows.tar.gz from the qt5-install folder.

Using 7-Zip:  
* `cd` to the *qt5* folder.  
* `7z a -ttar qt5-install-5.15.2-windows.tar qt5-install`  
* `7z a -tgzip qt5-install-5.15.2-windows.tar.gz qt5-install-5.15.2-windows.tar`

Upload qt5-install-5.15.2-windows.tar.gz to the Amazon S3 vircadia-public bucket, under the dependencies/vckpg directory.
Update hifi_vcpkg.py to use this new URL. Additionally, you should make a small change to any file in the vircadia/cmake/ports 
directory to force the re-download of the qt-install.tar.gz during the build process for Vircadia.

#### Preparing Symbols

Using Python 3, Run the following command in the *qt5* folder, substituting in paths as required. The 
*prepare-windows-symbols-for-backtrace.py* file is in the Vircadia repository's *tools\qt-builder* folder.  
`<path>python <path>prepare-windows-symbols-for-backtrace.py qt5-install` 

This scans the qt5-install directory for dlls and pdbs,  and copies them to a *backtrace* directory. 
Check that all dlls and pdbs are in the root of the directory.  

Zip up this directory and upload it to Backtrace or other crash log handlng tool.


### Linux
**TODO: Update this section for Qt 5.15.2**  

#### Preparing source files
`git clone --recursive git://code.qt.io/qt/qt5.git -b 5.12.3 --single-branch`  
  
*  Copy the **patches** folder to qt5  
*   Apply patches to Qt  
`cd qt5`  
`git apply --ignore-space-change --ignore-whitespace patches/aec.patch`  
`git apply --ignore-space-change --ignore-whitespace patches/qtscript-crash-fix.patch`  
`cd ..`

#### Configuring
`mkdir qt5-install`  
`mkdir qt5-build`  
`cd qt5-build`  

*Ubuntu 16.04*  
`../qt5/configure -opensource -confirm-license -platform linux-g++-64 -qt-zlib -qt-libjpeg -qt-libpng -qt-xcb -qt-freetype -qt-pcre -qt-harfbuzz -nomake examples -nomake tests -skip qttranslations -skip qtserialport -skip qt3d -skip qtlocation -skip qtwayland -skip qtsensors -skip qtgamepad -skip qtspeech -skip qtcharts -skip qtmacextras -skip qtvirtualkeyboard -skip qtpurchasing -skip qtdatavis3d -no-warnings-are-errors -no-pch -no-egl -no-icu -prefix ../qt5-install`  
**TODO: Remove `-skip qtspeech`**

*Ubuntu 18.04*  
`../qt5/configure -force-debug-info -release -opensource -confirm-license -gdb-index -recheck-all -nomake tests -nomake examples -skip qttranslations -skip qtserialport -skip qt3d -skip qtlocation -skip qtwayland -skip qtsensors -skip qtgamepad -skip qtspeech -skip qtcharts -skip qtx11extras -skip qtmacextras -skip qtvirtualkeyboard -skip qtpurchasing -skip qtdatavis3d -no-warnings-are-errors -no-pch -c++std c++14 -prefix ../qt5-install`  
**TODO: Remove `-skip qtspeech`**


???`../qt5/configure -opensource -confirm-license -gdb-index -nomake examples -nomake tests -skip qttranslations -skip qtserialport -skip qt3d -skip qtlocation -skip qtwayland -skip qtsensors -skip qtgamepad -skip qtspeech -skip qtcharts -skip qtmacextras -skip qtvirtualkeyboard -skip qtpurchasing -skip qtdatavis3d -no-warnings-are-errors -no-pch -prefix ../qt5-install`  

#### Make  
`make`  

????*Ubuntu 18.04 only*  
????`make module-qtwebengine`  
????`make module-qtscript`  

*Both*  
`make install`  

#### Fixing
1.  The *.prl* files have an absolute path that needs to be removed (see http://www.linuxfromscratch.org/blfs/view/stable-systemd/x/qtwebengine.html)  
`cd ../qt5-install`  
`find . -name \*.prl -exec sed -i -e '/^QMAKE_PRL_BUILD_DIR/d' {} \;`  
1.   Copy *qt.conf* to *qt5-install\bin*  

#### Uploading  
*Ubuntu 16.04*  
1.  Return to the home folder    
`cd ..`  
1.  Open a python 3 shell  
`python3`  
1.  Run the following snippet:  
`import os`  
`import tarfile`  
`filename=tarfile.open("qt5-install.tar.gz", "w:gz")`    
`filename.add("qt5-install", os.path.basename("qt5-install"))`    
`exit()`  
1.  Upload qt5-install.tar.gz to https://hifi-qa.s3.amazonaws.com/qt5/Ubuntu/16.04    

*Ubuntu 18.04*  
``tar -zcvf qt5-install.tar.gz qt5-install`  
1.  Upload qt5-install.tar.gz to https://hifi-qa.s3.amazonaws.com/qt5/Ubuntu/18.04  


### Mac  

#### Preparing source files  
git clone --recursive git://code.qt.io/qt/qt5.git -b 5.15.2 --single-branch    

*  If you are compiling with MacOSX11.1.SDK or greater, edit qt5/qtwebengine/src/3rdparty/chromium/build/mac/find_sdk.py line 91 and replace "MacOSX(10" with "MacOSX(11".

#### Configuring
`mkdir qt5-install`  
`mkdir qt5-build`  
`cd ../qt5-build`  

`../qt5/configure -force-debug-info -opensource -confirm-license -qt-zlib -qt-libjpeg -qt-libpng -qt-freetype -qt-pcre -qt-harfbuzz -nomake examples -nomake tests -skip qttranslations -skip qtserialport -skip qt3d -skip qtlocation -skip qtwayland -skip qtsensors -skip qtgamepad -skip qtcharts -skip qtx11extras -skip qtmacextras -skip qtvirtualkeyboard -skip qtpurchasing -skip qtdatavis3d -no-warnings-are-errors  -no-pch -prefix ../qt5-install`  

#### Make
`make`  
`make install`  

#### Fixing
1.  The *.prl* files have an absolute path that needs to be removed (see http://www.linuxfromscratch.org/blfs/view/stable-systemd/x/qtwebengine.html)  
`cd` to the `qt5-install directory`  
`find . -name \*.prl -exec sed -i -e '/^QMAKE_PRL_BUILD_DIR/d' {} \;`  
`cd ..`  
1.   Note: you may have additional files in qt5-install/lib and qt5-install/lib/pkg/pkgconfig that have your local build absolute path included.  Optionally you can fix these as well, but it will not effect the build if left alone.

Add a *qt.conf* file.  
1. Copy the file *qt5-build\qtbase\bin\qt.conf* to *qt5-install\bin*
1. Edit the *qt.conf* file: replace all absolute URLs with relative URLs (beginning with .. or .)

#### Uploading
`tar -zcvf qt5-install-5.15.2-macos.tar.gz qt5-install`  
Upload qt5-install-5.15.2-macos.tar.gz to our Amazon S3 vircadia-public bucket, under the dependencies/vckpg directory

#### Creating symbols (optional)
Run `python3 prepare-mac-symbols-for-backtrace.py qt5-install` to scan the qt5-build directory for any dylibs and execute dsymutil to create dSYM bundles.  After running this command the backtrace directory will be created.  Zip this directory up, but make sure that all dylibs and dSYM fiels are in the root of the zip file, not under a sub-directory.  This file can then be uploaded to backtrace or other crash log handling tool.

## Problems
*configure* errors, if any, may be viewed in **config.log** and **config.summary**
