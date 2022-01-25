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
1.  qt5 requirements
On Ubuntu based systems you can just have apt install the dependencies for the qt5-default package:
Edit /etc/apt/sources.list (edit as root) and uncomment all source repositories by replacing all `# deb-src` with `deb-src`.
```bash
sudo apt update
sudo apt upgrade
sudo apt build-dep qt5-default
```

2.  git >= 1.6
3.  python 2.7.x
4.  gperf
5.  bison and flex
6.  pkg-config (needed for qtwebengine)
7.  OpenGL
8.  make
9.  g++
10.  dbus-1 (needed for qtwebengine)
11.  nss (needed for qtwebengine)

On Ubuntu based systems you can install all these dependencies with:
```bash
sudo apt install git python gperf flex bison pkg-config mesa-utils libgl1-mesa-dev make g++ libdbus-glib-1-dev libnss3-dev
```
Make sure that python 2.7.x is the system standard by checking if `python --version` returns python 2.7.x

Qt also provides a dependency list for some major Linux distributions here: https://wiki.qt.io/Building_Qt_5_from_Git#Linux.2FX11

### Mac
1. macOS Catalina or higher
1.  git >= 1.6
 Check if needed `git --version`
 Install from https://git-scm.com/download/mac
 Verify again
1.  install pkg-config, dbug-glib, and fontconfig
 `brew install fontconfig dbus-glib pkg-config`
1. Xcode 10.3 or higher
 You can get different Xcode version from https://xcodereleases.com
1. macOS may install an incompatible Xcode command line tools version. If you run into weird issues, you may need to delete your current command line tools and replace it with an older version.
 This happens on macOS Catalina.
 `sudo rm -rf /Library/Developer/CommandLineTools`
 Download Command Line Tools for Xcode 11.3.1 from https://developer.apple.com/download/more/ and install said Command Line Tools.
 The versions don't have to match. Keep in mind that macOS will prompt you to install "system updates" which include the broken version of Xcode Command Line Tools.
 If you do run into an issue you think might be related, remember to clear the *qt5-build* as well as the *qt5-install* directory.

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
#### Preparing source files
```bash
git clone --recursive git://code.qt.io/qt/qt5.git -b 5.15.2 --single-branch
```

#### Configuring
```bash
mkdir qt5-install
mkdir qt5-build
cd qt5-build
```

amd64:
```bash
../qt5/configure -force-debug-info -release -opensource -confirm-license -platform linux-g++-64 -recheck-all -nomake tests -nomake examples -skip qttranslations -skip qtserialport -skip qt3d -skip qtlocation -skip qtwayland -skip qtsensors -skip qtgamepad -skip qtcharts -skip qtx11extras -skip qtmacextras -skip qtvirtualkeyboard -skip qtpurchasing -skip qtdatavis3d -skip qtlottie -skip qtquick3d -skip qtpim -skip qtdocgallery -no-warnings-are-errors -no-pch -no-egl -no-icu -prefix ../qt5-install
```
If libX11 or libGL aren't found, you will need to manually provide those locations. Search for `libX11.so` and `libGL.so` respectively and provide paths to their folders inside `qt5/qtbase/mkdspecs/linux-g++-64/qmake.conf`.
On Ubuntu 18.04 both are in `/usr/lib/x86_64-linux-gnu`

aarch64:
```bash
../qt5/configure -force-debug-info -release -opensource -confirm-license -platform linux-g++ -recheck-all -nomake tests -nomake examples -skip qttranslations -skip qtserialport -skip qt3d -skip qtlocation -skip qtwayland -skip qtsensors -skip qtgamepad -skip qtcharts -skip qtx11extras -skip qtmacextras -skip qtvirtualkeyboard -skip qtpurchasing -skip qtdatavis3d -skip qtlottie -skip qtquick3d -skip qtpim -skip qtdocgallery -no-warnings-are-errors -no-pch -no-icu -prefix ../qt5-install
```
You can accelerate the build process by installing some of the optional system dependencies.

#### Make
Replace `4` with the number of threads you want to use. Keep in mind that the Qt build process uses a lot of memory. It is recommended to have at least 1.2 GiB per thread.
```bash
NINJAFLAGS='-j4' make -j4
```

On some systems qtscript, qtwebengine, qtwebchannel and qtspeech will be build by `make` without you manually specifying it, while others require you to build modules individually.
Check the folders of each needed module for binary files, to make sure that they have actually been built.
If a module has not been built, make the module "manually":
```bash
make -j4 module-qtscript
NINJAFLAGS='-j4' make -j4 module-qtwebengine
make -j4 module-qtspeech
make -j4 module-qtwebchannel
```

Now Qt can be installed to qt5-install:
```bash
make -j4 install
```

If some modules were missing in previous steps, it is needed to install them individually as well:
```bash
cd qtwebengine
make -j4 install
cd ../qtscript
make -j4 install
cd ../qtspeech
make -j4 install
cd ../qtwebchannel
make -j4 install
```
If one of the make commands fails, running it a second time sometimes clears the issue.

#### Fixing
1.  The *.prl* files have an absolute path that needs to be removed (see http://www.linuxfromscratch.org/blfs/view/stable-systemd/x/qtwebengine.html)
```bash
cd ../qt5-install
find . -name \*.prl -exec sed -i -e '/^QMAKE_PRL_BUILD_DIR/d' {} \;
```
2.   Copy *qt.conf* to *qt5-install\bin*

#### Uploading
1.  Tar and xz qt5-install to create the package. Replace `ubuntu-18.04` with the relevant system and `amd64` with the relevant architecture.
```bash
tar -Jcvf qt5-install-5.15.2-ubuntu-18.04-amd64.tar.xz qt5-install
```
2.  Upload qt5-install-5.15.2-ubuntu-18.04-amd64.tar.xz to https://athena-public.s3.amazonaws.com/dependencies/vcpkg/



### Mac
#### Preparing source files
```bash
git clone --recursive git://code.qt.io/qt/qt5.git -b 5.15.2 --single-branch
```

Plain Qt 5.15.2 cannot actually be built on most supported configurations. To fix this, we will use QtWebEngine from Qt 5.15.7.
```bash
cd qt5/qtwebengine
git pull git://code.qt.io/qt/qtwebengine.git 5.15.7
git submodule update
cd ../..
```

#### Configuring
Note: If you run into any issues with Qt on macOS, take a look at what our friends at macports are doing.
- https://github.com/macports/macports-ports/tree/master/aqua/qt5/files
- https://trac.macports.org/query?status=accepted&status=assigned&status=closed&status=new&status=reopened&port=~qt5&desc=1&order=id

```bash
mkdir qt5-install
mkdir qt5-build
cd qt5-build
```

```bash
../qt5/configure -force-debug-info -release -opensource -confirm-license -qt-zlib -qt-libjpeg -qt-libpng -qt-freetype -qt-pcre -qt-harfbuzz -recheck-all -nomake tests  -nomake examples -skip qttranslations -skip qtserialport -skip qt3d -skip qtlocation -skip qtwayland -skip qtsensors -skip qtgamepad -skip qtcharts -skip qtx11extras -skip qtmacextras -skip qtvirtualkeyboard -skip qtpurchasing -skip qtdatavis3d -skip qtlottie -skip qtquick3d -skip qtpim -skip qtdocgallery -no-warnings-are-errors  -no-pch -no-egl -no-icu -prefix ../qt5-install
```

#### Make
Important: Building Qt using multiple threads needs a lot of system memory in later stages of the process. You should have around 1.5GiB available per thread you intend to use.
```bash
NINJAFLAGS='-j4'  make -j4
```

The Qt documentation states that using more than one thread on the install step can cause problems on macOS.
```bash
make -j1 install
```

#### Fixing
1.  Building with newer QtWebEngine will fail by standard. To fix this we need to change the relevant cmake files in `qt5-install`. (See https://www.qt.io/blog/building-qt-webengine-against-other-qt-versions and https://github.com/macports/macports-ports/pull/12595/files )
    - `cd` to the `qt5-install` directory.
    - Enter `bash` since zsh cannot run the following command.
    - `find . -name \Qt5WebEngine*Config.cmake -exec sed -i '' -e 's/5\.15\.7/5\.15\.2/g' {} \;`
    - `exit` bash to got back to zsh.
    - `cd ..`

2.  The *.prl* files have an absolute path that needs to be removed (see http://www.linuxfromscratch.org/blfs/view/stable-systemd/x/qtwebengine.html)
    - `cd` to the `qt5-install` directory
    - `find . -name \*.prl -exec sed -i -e '/^QMAKE_PRL_BUILD_DIR/d' {} \;`
    - `cd ..`
3.   Note: you may have additional files in qt5-install/lib and qt5-install/lib/pkg/pkgconfig that have your local build absolute path included.  Optionally you can fix these as well, but it will not effect the build if left alone.

Add a *qt.conf* file.
1. Copy the file *qt5-build\qtbase\bin\qt.conf* to *qt5-install\bin*
1. Edit the *qt.conf* file: replace all absolute URLs with relative URLs (beginning with .. or .)

#### Uploading
```bash
tar -Jcvf qt5-install-5.15.2-qtwebengine-5.15.7-macOSXSDK10.14-macos.tar.xz qt5-install
```
Upload qt5-install-5.15.2-qtwebengine-5.15.7-macOSXSDK10.14-macos.tar.xz to our Amazon S3 vircadia-public bucket, under the dependencies/vckpg directory

#### Creating symbols (optional)
Run `python3 prepare-mac-symbols-for-backtrace.py qt5-install` to scan the qt5-build directory for any dylibs and execute dsymutil to create dSYM bundles.  After running this command the backtrace directory will be created.  Zip this directory up, but make sure that all dylibs and dSYM fiels are in the root of the zip file, not under a sub-directory.  This file can then be uploaded to backtrace or other crash log handling tool.

## Problems
*configure* errors, if any, may be viewed in **config.log** and **config.summary**
