Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only Windows specific instructions are found in this file.

###Windows Specific Dependencies
* [GLEW](http://glew.sourceforge.net/) ~> 1.10.0
* [freeglut MSVC](http://www.transmissionzero.co.uk/software/freeglut-devel/) ~> 2.8.1
* [zLib](http://www.zlib.net/) ~> 1.2.8
* (remember that you need all other dependencies listed in [BUILD.md](BUILD.md))

####Visual Studio 2013

You can use the Community or Professional editions of Visual Studio 2013.

You can start a Visual Studio 2013 command prompt using the shortcut provided in the Visual Studio Tools folder installed as part of Visual Studio 2013.

Or you can start a regular command prompt and then run:

    "%VS120COMNTOOLS%\vsvars32.bat"

#####Windows SDK 8.1

If using Visual Studio 2013 and building as a Visual Studio 2013 project you need the Windows 8 SDK which you should already have as part of installing Visual Studio 2013. You should be able to see it at `C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\x86`.

###Qt
You can use the online installer or the offline installer. If you use the offline installer, be sure to select the "OpenGL" version.

NOTE: Qt does not support 64-bit builds on Windows 7, so you must use the 32-bit version of libraries for interface.exe to run. The 32-bit version of the static library is the one linked by our CMake find modules.

* [Download the online installer](http://qt-project.org/downloads)
    * When it asks you to select components, ONLY select the following:
        * Qt > Qt 5.3.2 > **msvc2013 32-bit OpenGL**

* [Download the offline installer](http://download.qt-project.org/official_releases/qt/5.3/5.3.2/qt-opensource-windows-x86-msvc2013_opengl-5.3.2.exe)

Once Qt is installed, you need to manually configure the following:
* Make sure the Qt runtime DLLs are loadable. You must do this before you attempt to build because some tools for the build depend on Qt. E.g., add to the PATH: `Qt\5.3.2\msvc2013_opengl\bin\`. 
* Go to Control Panel > System > Advanced System Settings > Environment Variables > New ...
* Set the QT_CMAKE_PREFIX_PATH environment variable to your `Qt\5.3.2\msvc2013_opengl` directory.

###External Libraries

As it stands, Hifi/Interface is a 32-bit application, so all libraries should also be 32-bit.

CMake will need to know where the headers and libraries for required external dependencies are. 

The recommended route for CMake to find the external dependencies is to place all of the dependencies in one folder and set one ENV variable - HIFI_LIB_DIR. That ENV variable should point to a directory with the following structure:

    root_lib_dir
        -> bullet
            -> include
            -> lib
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
        -> tbb
            -> include
            -> lib
        -> zlib
           -> include
           -> lib
           -> test

For many of the external libraries where precompiled binaries are readily available you should be able to simply copy the extracted folder that you get from the download links provided at the top of the guide. Otherwise you may need to build from source and install the built product to this directory. The `root_lib_dir` in the above example can be wherever you choose on your system - as long as the environment variable HIFI_LIB_DIR is set to it. From here on, whenever you see %HIFI_LIB_DIR% you should substitute the directory that you chose.

As with the Qt libraries, you will need to make sure that directories containing DLL'S are in your path. Where possible, you can use static builds of the external dependencies to avoid this requirement.

###OpenSSL

Qt will use OpenSSL if it's available, but it doesn't install it, so you must install it separately.

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
* Win32 OpenSSL v1.0.1L

Install OpenSSL into the Windows system directory, to make sure that Qt uses the version that you've just installed, and not some other version.

###Intel Threading Building Blocks (TBB)

Download the zip from the [TBB website](https://www.threadingbuildingblocks.org/). 

We recommend you extract it to %HIFI_LIB_DIR%\tbb. This will help our FindTBB cmake module find what it needs. You can place it wherever you like on your machine if you specify TBB_ROOT_DIR as an environment variable or a variable passed when cmake is run.

###Zlib

Download the compiled DLL from the [zlib website](http://www.zlib.net/). Extract to %HIFI_LIB_DIR%\zlib.

Add the following environment variables (remember to substitute your own directory for %HIFI_LIB_DIR%):

    ZLIB_LIBRARY=%HIFI_LIB_DIR%\zlib\lib\zdll.lib
    ZLIB_INCLUDE_DIR=%HIFI_LIB_DIR%\zlib\include

Add to the PATH: `%HIFI_LIB_DIR%\zlib`

(The PATH environment variable is where Windows looks for its DLL's and executables. There's a great tool for editing these variables with ease, [Rapid Environment Editor](http://www.rapidee.com/en/download))

Important! This should be added at the beginning of the path, not the end (your 
system likely has many copies of zlib1.dll, and you want High Fidelity to use the correct version). If High Fidelity picks up the wrong zlib1.dll then it might be unable to use it, and that would cause it to fail to start, showing only the cryptic error "The application was unable to start correctly: 0xc0000022".

###freeglut

Download the binary package: `freeglut-MSVC-2.8.1-1.mp.zip`. Extract to %HIFI_LIB_DIR%\freeglut.

Add to the PATH: `%HIFI_LIB_DIR%\freeglut\bin`

###GLEW

Download the binary package: `glew-1.10.0-win32.zip`. Extract to %HIFI_LIB_DIR%\glew (you'll need to rename the default directory name).

Add to the PATH: `%HIFI_LIB_DIR%\glew\bin\Release\Win32`

###GLM

This package contains only headers, so there's nothing to add to the PATH.

Be careful with glm. For the folder other libraries would normally call 'include', the folder containing the headers, glm opts to use 'glm'. You will have a glm folder nested inside the top-level glm folder.

###Gverb

1. Go to https://github.com/highfidelity/gverb
   Or download the sources directly via this link: 
   https://github.com/highfidelity/gverb/archive/master.zip

2. Extract the archive

3. Place the directories “include” and “src” in interface/external/gverb 
   (Normally next to this readme)

4. Clear your build directory, run cmake, build and you should be all set.


###Bullet

Bullet 2.82 source can be [downloaded here](https://code.google.com/p/bullet/downloads/detail?name=bullet-2.82-r2704.zip). Bullet does not come with prebuilt libraries, you need to make those yourself.

* Download the zip file and extract into a temporary folder
* Create a directory named cmakebuild. Bullet comes with a build\ directory by default, however, that directory is intended for use with premake, and considering premake doesn't support VS2013, we prefer to run the cmake build on its own directory.
* Make the following modifications to Bullet's source:
   1. In file: Extras\HACD\hacdICHull.cpp --- in line: 17 --- insert: #include &lt;algorithm&gt;
   2. In file: src\MiniCL\cl_MiniCL_Defs.h --- comment lines 364 to 372
   3. In file: CMakeLists.txt set to ON the option USE_MSVC_RUNTIME_LIBRARY_DLL in line 27

Then create the Visual Studio solution and build the libraries - run the following commands from a Visual Studio 2013 command prompt, from within the cmakebuild directory created before:

```shell
cmake .. -G "Visual Studio 12"
msbuild BULLET_PHYSICS.sln /p:Configuration=Debug
```

This will create Debug libraries in cmakebuild\lib\Debug you can replace Debug with Release in the msbuild command and that will generate Release libraries in cmakebuild\lib\Release.

You now have Bullet libraries compiled, now you need to put them in the right place for hifi to find them:

* Create a directory named bullet\ inside your %HIFI_LIB_DIR%
* Create two directores named lib\ and include\ inside bullet\
* Copy all the contents inside src\ from the bullet unzip path into %HIFI_LIB_DIR%\bullet\include\
* Copy all the contents inside cmakebuild\lib\ into %HIFI_LIB_DIR\bullet\lib

_Note that the INSTALL target should handle the copying of files into an install directory automatically, however, without modifications to Cmake, the install target didn't work right for me, please update this instructions if you get that working right - Leo &lt;leo@highfidelity.io&gt;_

###Build High Fidelity using Visual Studio
Follow the same build steps from the CMake section of [BUILD.md](BUILD.md), but pass a different generator to CMake.

    cmake .. -DZLIB_LIBRARY=%ZLIB_LIBRARY% -DZLIB_INCLUDE_DIR=%ZLIB_INCLUDE_DIR% -G "Visual Studio 12"

Open %HIFI_DIR%\build\hifi.sln and compile.

###Running Interface
If you need to debug Interface, you can run interface from within Visual Studio (see the section below). You can also run Interface by launching it from command line or File Explorer from %HIFI_DIR%\build\interface\Debug\interface.exe

###Debugging Interface
* In the Solution Explorer, right click interface and click Set as StartUp Project
* Set the "Working Directory" for the Interface debugging sessions to the Debug output directory so that your application can load resources. Do this: right click interface and click Properties, choose Debugging from Configuration Properties, set Working Directory to .\Debug
* Now you can run and debug interface through Visual Studio
