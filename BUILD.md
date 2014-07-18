Dependencies
===
* [cmake](http://www.cmake.org/cmake/resources/software.html) ~> 2.8.12.2
* [Qt](http://qt-project.org/downloads) ~> 5.2.0
* [glm](http://glm.g-truc.net/0.9.5/index.html) ~> 0.9.5.2
* [OpenSSL](https://www.openssl.org/related/binaries.html) ~> 1.0.1g
  * IMPORTANT: OpenSSL 1.0.1g is critical to avoid a security vulnerability.

#####Linux only
* [freeglut](http://freeglut.sourceforge.net/) ~> 2.8.0
* [zLib](http://www.zlib.net/) ~> 1.2.8

#####Windows only
* [GLEW](http://glew.sourceforge.net/) ~> 1.10.0
* [freeglut MSVC](http://www.transmissionzero.co.uk/software/freeglut-devel/) ~> 2.8.1
* [zLib](http://www.zlib.net/) ~> 1.2.8

CMake
=== 
Hifi uses CMake to generate build files and project files for your platform.

####Qt
In order for CMake to find the Qt5 find modules, you will need to set an ENV variable pointing to your Qt installation.

For example, a Qt5 5.2.0 installation to /usr/local/qt5 would require that QT_CMAKE_PREFIX_PATH be set with the following command. This can either be entered directly into your shell session before you build or in your shell profile (e.g.: ~/.bash_profile, ~/.bashrc, ~/.zshrc - this depends on your shell and environment).

The path it needs to be set to will depend on where and how Qt5 was installed. e.g.

    export QT_CMAKE_PREFIX_PATH=/usr/local/qt/5.2.0/clang_64/lib/cmake/
    export QT_CMAKE_PREFIX_PATH=/usr/local/Cellar/qt5/5.2.1/lib/cmake
    export QT_CMAKE_PREFIX_PATH=/usr/local/opt/qt5/lib/cmake

####Generating build files
Create a build directory in the root of your checkout and then run the CMake build from there. This will keep the rest of the directory clean.

    mkdir build
    cd build
    cmake ..

####Variables
Any variables that need to be set for CMake to find dependencies can be set as ENV variables in your shell profile, or passed directly to CMake with a `-D` flag appended to the `cmake ..` command.

For example, to pass the QT_CMAKE_PREFIX_PATH variable during build file generation:

    cmake .. -DQT_CMAKE_PREFIX_PATH=/usr/local/qt/5.2.1/lib/cmake

####Finding Dependencies
You can point our [Cmake find modules](cmake/modules/) to the correct version of dependencies by setting one of the three following variables to the location of the correct version of the dependency.

In the examples below the variable $NAME would be replaced by the name of the dependency in uppercase, and $name would be replaced by the name of the dependency in lowercase (ex: OPENSSL_ROOT_DIR, openssl).

* $NAME_ROOT_DIR - pass this variable to Cmake with the -DNAME_ROOT_DIR= flag when running Cmake to generate build files
* $NAME_ROOT_DIR - set this variable in your ENV
* HIFI_LIB_DIR - set this variable in your ENV to your High Fidelity lib folder, should contain a folder '$name'

UNIX
===
In general, as long as external dependencies are placed in OS standard locations, CMake will successfully find them during its run. When possible, you may choose to install depencies from your package manager of choice, or from source. 

####Linux
Should you choose not to install Qt5 via a package manager that handles dependencies for you, you may be missing some Qt5 dependencies. On Ubuntu, for example, the following additional packages are required:

    libasound2 libxmu-dev libxi-dev freeglut3-dev libasound2-dev libjack-dev

####OS X
#####Package Managers
[Homebrew](http://brew.sh/) is an excellent package manager for OS X. It makes install of all hifi dependencies very simple.

    brew tap highfidelity/homebrew-formulas
    brew install cmake glm openssl
    brew install highfidelity/formulas/qt5
    brew link qt5 --force
    brew install highfidelity/formulas/qxmpp

We have a [homebrew formulas repository](https://github.com/highfidelity/homebrew-formulas) that you can use/tap to install some of the dependencies. In the code block above qt5 and qxmpp are installed from formulas in this repository.

*Our [qt5 homebrew formula](https://raw.github.com/highfidelity/homebrew-formulas/master/qt5.rb) is for a patched version of Qt 5.2.0 stable that removes wireless network scanning that can reduce real-time audio performance. We recommended you use this formula to install Qt.*

#####Xcode
If Xcode is your editor of choice, you can ask CMake to generate Xcode project files instead of Unix Makefiles.

    cmake .. -GXcode

After running cmake, you will have the make files or Xcode project file necessary to build all of the components. Open the hifi.xcodeproj file, choose ALL_BUILD from the Product > Scheme menu (or target drop down), and click Run.

If the build completes successfully, you will have built targets for all components located in the `build/${target_name}/Debug` directories.

Windows
===
####Visual Studio

Currently building on Windows has been tested using the following compilers:
* Visual Studio C++ 2010 Express
* Visual Studio 2013

(If anyone can test using Visual Studio 2013 Express then please update this document)

#####Windows SDK 7.1

Whichever version of Visual Studio you use, first install [Microsoft Windows SDK for Windows 7 and .NET Framework 4](http://www.microsoft.com/en-us/download/details.aspx?id=8279).

#####Visual Studio C++ 2010 Express

Visual Studio C++ 2010 Express can be downloaded [here](http://www.visualstudio.com/en-us/downloads#d-2010-express).

The following patches/service packs are also required:
* [VS2010 SP1](http://www.microsoft.com/en-us/download/details.aspx?id=23691)
* [VS2010 SP1 Compiler Update](http://www.microsoft.com/en-us/download/details.aspx?id=4422)

Some of the build instructions will ask you to start a Visual Studio Command Prompt. You should have a shortcut in your Start menu called "Open Visual Studio Command Prompt (2010)" which will do so.

#####Visual Studio 2013

This product must be purchased separately.

Visual Studio 2013 doesn't have a shortcut to start a Visual Studio Command Prompt. Instead, start a regular command prompt and then run:

    "%VS120COMNTOOLS%\vsvars32.bat"

####Qt
You can use the online installer or the offline installer. If you use the offline installer, be sure to select the "OpenGL" version.

NOTE: Qt does not support 64-bit builds on Windows 7, so you must use the 32-bit version of libraries for interface.exe to run. The 32-bit version of the static library is the one linked by our CMake find modules.

* Download the online installer [here](http://qt-project.org/downloads)
    * When it asks you to select components, ONLY select the following:
        * Qt > Qt 5.2.0 > **msvc2010 32-bit OpenGL**

* Download the offline installer [here](http://download.qt-project.org/official_releases/qt/5.2/5.2.0/qt-windows-opensource-5.2.0-msvc2010_opengl-x86-offline.exe)

Once Qt is installed, you need to manually configure the following:
* Make sure the Qt runtime DLLs are loadable. You must do this before you attempt to build because some tools for the build depend on Qt. E.g., add to the PATH: `Qt\5.2.0\msvc2010_opengl\bin\`. 
* Set the QT_CMAKE_PREFIX_PATH environment variable to your `Qt\5.2.0\msvc2010_opengl` directory.

####External Libraries

CMake will need to know where the headers and libraries for required external dependencies are. 

The recommended route for CMake to find the external dependencies is to place all of the dependencies in one folder and set one ENV variable - HIFI_LIB_DIR. That ENV variable should point to a directory with the following structure:

    root_lib_dir
        -> freeglut
            -> bin
            -> include
            -> lib
        -> glew
            -> bin
            -> include
            -> lib
        -> glm
            -> glm
                -> glm.hpp
        -> openssl
            -> bin
            -> include
            -> lib
        -> zlib
           -> include
           -> lib
           -> test

For many of the external libraries where precompiled binaries are readily available you should be able to simply copy the extracted folder that you get from the download links provided at the top of the guide. Otherwise you may need to build from source and install the built product to this directory. The `root_lib_dir` in the above example can be wherever you choose on your system - as long as the environment variable HIFI_LIB_DIR is set to it. From here on, whenever you see %HIFI_LIB_DIR% you should substitute the directory that you chose.

As with the Qt libraries, you will need to make sure that directories containing DLL'S are in your path. Where possible, you can use static builds of the external dependencies to avoid this requirement.

#### OpenSSL

QT will use OpenSSL if it's available, but it doesn't install it, so you must install it separately.

Your system may already have several versions of the OpenSSL DLL's (ssleay32.dll, libeay32.dll) lying around, but they may be the wrong version. If these DLL's are in the PATH then QT will try to use them, and if they're the wrong version then you will see the following errors in the console:

    QSslSocket: cannot resolve TLSv1_1_client_method
    QSslSocket: cannot resolve TLSv1_2_client_method
    QSslSocket: cannot resolve TLSv1_1_server_method
    QSslSocket: cannot resolve TLSv1_2_server_method
    QSslSocket: cannot resolve SSL_select_next_proto
    QSslSocket: cannot resolve SSL_CTX_set_next_proto_select_cb
    QSslSocket: cannot resolve SSL_get0_next_proto_negotiated

To prevent these problems, install OpenSSL yourself. Download the following binary packages [from this website](http://slproweb.com/products/Win32OpenSSL.html):
* Visual C++ 2008 Redistributables
* Win32 OpenSSL v1.0.1h

Install OpenSSL into the Windows system directory, to make sure that QT uses the version that you've just installed, and not some other version.

#### Zlib

Download the compiled DLL from the [zlib website](http://www.zlib.net/). Extract to %HIFI_LIB_DIR%\zlib.

Add the following environment variables (remember to substitute your own directory for %HIFI_LIB_DIR%):

    ZLIB_LIBRARY=%HIFI_LIB_DIR%\zlib\lib\zdll.lib
    ZLIB_INCLUDE_DIR=%HIFI_LIB_DIR%\zlib\include

Add to the PATH: `%HIFI_LIB_DIR%\zlib`

Important! This should be added at the beginning of the path, not the end. That's because your 
system likely has many copies of zlib1.dll, and you want High Fidelity to use the correct version. If High Fidelity picks up the wrong zlib1.dll then it might be unable to use it, and that would cause it to fail to start, showing only the cryptic error "The application was unable to start correctly: 0xc0000022".

#### freeglut

Download the binary package: `freeglut-MSVC-2.8.1-1.mp.zip`. Extract to %HIFI_LIB_DIR%\freeglut.

Add to the PATH: `%HIFI_LIB_DIR%\freeglut\bin`

#### GLEW

Download the binary package: `glew-1.10.0-win32.zip`. Extract to %HIFI_LIB_DIR%\glew (you'll need to rename the default directory name).

Add to the PATH: `%HIFI_LIB_DIR%\glew\bin\Release\Win32`

#### GLM

This package contains only headers, so there's nothing to add to the PATH.

Be careful with glm. For the folder other libraries would normally call 'include', the folder containing the headers, glm opts to use 'glm'. You will have a glm folder nested inside the top-level glm folder.

#### Build High Fidelity using Visual Studio
Follow the same build steps from the CMake section, but pass a different generator to CMake.

    cmake .. -DZLIB_LIBRARY=%ZLIB_LIBRARY% -DZLIB_INCLUDE_DIR=%ZLIB_INCLUDE_DIR% -G "Visual Studio 10"

If you're using Visual Studio 2013 then pass "Visual Studio 12" instead of "Visual Studio 10" (yes, 12, not 13).

Open %HIFI_DIR%\build\hifi.sln and compile.

####Running Interface
If you need to debug Interface, you can run interface from within Visual Studio (see the section below). You can also run Interface by launching it from command line or File Explorer from %HIFI_DIR%\build\interface\Debug\interface.exe

####Debugging Interface
* In the Solution Explorer, right click interface and click Set as StartUp Project
* Set the "Working Directory" for the Interface debugging sessions to the Debug output directory so that your application can load resources. Do this: right click interface and click Properties, choose Debugging from Configuration Properties, set Working Directory to .\Debug
* Now you can run and debug interface through Visual Studio

Optional Components
===

####QXmpp

You can find QXmpp [here](https://github.com/qxmpp-project/qxmpp). The inclusion of the QXmpp enables text chat in the Interface client.

#### Devices

You can support external input/output devices such as Leap Motion, Faceplus, Faceshift, PrioVR, MIDI, Razr Hydra and more by adding each individual SDK in the visible building path. Refer to the readme file available in each device folder in [interface/external/](interface/external) for the detailed explanation of the requirements to use the device.

