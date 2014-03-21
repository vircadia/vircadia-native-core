Dependencies
===
* [cmake](http://www.cmake.org/cmake/resources/software.html) ~> 2.8.11
* [Qt](http://qt-project.org/downloads) ~> 5.2.0
* [zLib](http://www.zlib.net/) ~> 1.2.8
* [glm](http://glm.g-truc.net/0.9.5/index.html) ~> 0.9.5.0
* [qxmpp](https://code.google.com/p/qxmpp/) ~> 0.7.6

#####Linux only
* [freeglut](http://freeglut.sourceforge.net/) ~> 2.8.0

#####Windows only
* [GLEW](http://glew.sourceforge.net/) ~> 1.10.0
* [freeglut MSVC](http://www.transmissionzero.co.uk/software/freeglut-devel/) ~> 2.8.1

CMake
=== 
Hifi uses CMake to generate build files and project files for your platform.

####Qt
In order for CMake to find the Qt5 find modules, you will need to set an ENV variable pointing to your CMake installation.

For example, a Qt5 5.2.0 installation to /usr/local/qt5 would require that QT_CMAKE_PREFIX_PATH be set with the following command. This can either be entered directly into your shell session before you build or in your shell profile (e.g.: ~/.bash_profile, ~/.bashrc, ~/.zshrc - this depends on your shell and environment).

    export QT_CMAKE_PREFIX_PATH=/usr/local/qt/5.2.0/clang_64/lib/cmake/

The path it needs to be set to will depend on where and how Qt5 was installed.

####Generating build files
Create a build directory in the root of your checkout and then run the CMake build from there. This will keep the rest of the directory clean.

    mkdir build
    cd build
    cmake ..

####Variables
Any variables that need to be set for CMake to find dependencies can be set as ENV variables in your shell profile, or passed directly to CMake with a `-D` flag appended to the `cmake ..` command.

For example, to pass the QT_CMAKE_PREFIX_PATH variable during build file generation:

    cmake .. -DQT_CMAKE_PREFIX_PATH=/usr/local/qt/5.2.0/clang_64/lib/cmake


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
    brew install cmake glm zlib
    brew install highfidelity/formulas/qt5
    brew link qt5 --force
    brew install highfidelity/formulas/qxmpp

We have a [homebrew formulas repository](https://github.com/highfidelity/homebrew-formulas) that you can use/tap to install some of the dependencies. In the code block above qt5 and qxmpp are installed from formulas in this repository.

*Our [qt5 homebrew formula](https://raw.github.com/highfidelity/homebrew-formulas/master/qt5.rb) is for a patched version of Qt 5.2.0 stable that removes wireless network scanning that can reduce real-time audio performance. We recommended you use this formula to install Qt.*

#####Xcode
If Xcode is your editor of choice, you can ask CMake to generate Xcode project files instead of Unix Makefiles.

    cmake .. -GXcode

After running cmake, you will have the make files or Xcode project file necessary to build all of the components. Open the hifi.xcodeproj file, choose ALL_BUILD from the Product > Scheme menu (or target drop down), and click Run.

If the build completes successfully, you will have built targets for all components located in the `build/${target_name}/Debug directories`.

Windows
===
Currently building on Windows has only been tested on Windows SDK 7.1 with Visual Studio C++ 2010 Express.

Visual Studio C++ 2010 Express can be downloaded [here](http://www.visualstudio.com/en-us/downloads#d-2010-express).

The following patches/service packs are also required:
* [Windows SDK 7.1/.NET 4 Framework](http://www.microsoft.com/en-us/download/details.aspx?id=8279)
* [VS2010 SP1](http://www.microsoft.com/en-us/download/details.aspx?id=23691)
* [VS2010 SP1 Compiler Update](http://www.microsoft.com/en-us/download/details.aspx?id=4422)

####Qt
You can use the online installer, or the offline installer. If you use the offline installer, be sure to select the "OpenGL" version.
* Download the online installer [here](http://qt-project.org/downloads)
    * When it asks you to select components, ONLY select the following:
        * Qt > Qt 5.2.0 > **msvc2010 32-bit OpenGL**

* Download the offline installer [here](http://download.qt-project.org/official_releases/qt/5.2/5.2.0/qt-windows-opensource-5.2.0-msvc2010_opengl-x86-offline.exe)

Once Qt is installed, you need to manually configure the following:
* Make sure the Qt runtime DLLs are loadable (You could add the Qt\5.2.0\msvc2010_opengl\bin\ directory to your path.) - You must do this before you attempt to build because some tools for the build depend on Qt.
* Set the QT_CMAKE_PREFIX_PATH environment variable to your Qt\5.2.0\msvc2010_opengl directory

####zLib
NOTE: zLib should configure itself correctly on install. However, sometimes zLib doesn't properly detect system components and fails to configure itself correctly. When it fails, it will not correctly set the #if HAVE_UNISTD_H at line 287 of zconf.h to #if 0... if it fails, you're build will have errors in the voxels target. You can correct this by setting the #if to 0 instead of 1, since Windows does not have unistd.h.

####External Libraries
We don't currently have a Windows installer, so before running Interface, you will need to ensure that all required resources are loadable. 

CMake will need to know where the headers and libraries for required external dependencies are. If you installed ZLIB using the installer, the FindZLIB cmake module will be able to find it. This isn't the case for the others.

The recommended route for CMake to find external dependencies is to place all of the dependencies in one folder and set one ENV variable - HIFI_LIB_DIR. That ENV variable should point to a directory with the following structure:

    root_lib_dir
        -> glm
            -> glm
                -> glm.hpp
        -> glew
            -> bin
            -> include
            -> lib
        -> freeglut
            -> bin
            -> include
            -> lib
        -> qxmpp
            -> include
            -> lib

*NOTE: Be careful with glm. For the folder other libraries would normally call 'include', the folder containing the headers, glm opts to use 'glm'. You will have a glm folder nested inside the top-level glm folder.*

For many of the external libraries where precompiled binaries are readily available you should be able to simply copy the extracted folder that you get from the download links provided at the top of the guide. Otherwise you may need to build from source and install the built product to this directory. The `root_lib_dir` in the above example can be wherever you choose on your system - as long as the environment variable HIFI_LIB_DIR is set to it.

*NOTE: Qt does not support 64-bit builds on Windows 7, so you must use the 32-bit version of libraries for interface.exe to run. The 32-bit version of the static library is the one linked by our CMake find modules*

#### DLLs
As with the Qt libraries, you will need to make sure the directory containing dynamically-linked libraries is in your path. For example, for a dynamically linked build of freeglut, the directory to add to your path in which the DLL is found is `FREEGLUT_DIR/bin`. Where possible, you can use static builds of the external dependencies to avoid this requirement.

####Building in Visual Studio
Follow the same build steps from the CMake section, but pass a different generator to CMake.

    cmake .. -G "Visual Studio 10"

####Running Interface
If you need to debug Interface, you can run interface from within Visual Studio (see the section below). You can also run Interface by launching it from command line or File Explorer from $YOUR_HIFI_PATH\build\interface\Debug\interface.exe

####Debugging Interface
* In the Solution Explorer, right click interface and click Set as StartUp Project
* Set the "Working Directory" for the Interface debugging sessions to the Debug output directory so that your application can load resources. Do this: right click interface and click Properties, choose Debugging from Configuration Properties, set Working Directory to .\Debug
* Now you can run and debug interface through Visual Studio