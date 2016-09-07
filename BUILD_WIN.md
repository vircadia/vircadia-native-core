Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only Windows specific instructions are found in this file.

Interface can be built as 32 or 64 bit.

###Visual Studio 2013

You can use the Community or Professional editions of Visual Studio 2013.

You can start a Visual Studio 2013 command prompt using the shortcut provided in the Visual Studio Tools folder installed as part of Visual Studio 2013.

Or you can start a regular command prompt and then run:

    "%VS120COMNTOOLS%\vsvars32.bat"

####Windows SDK 8.1

If using Visual Studio 2013 and building as a Visual Studio 2013 project you need the Windows 8 SDK which you should already have as part of installing Visual Studio 2013. You should be able to see it at `C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\x86`.

####nmake

Some of the external projects may require nmake to compile and install. If it is not installed at the location listed below, please ensure that it is in your PATH so CMake can find it when required.

We expect nmake.exe to be located at the following path.

    C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin

###Qt
You can use the online installer or the offline installer. If you use the offline installer, be sure to select the "OpenGL" version.

* [Download the online installer](http://www.qt.io/download-open-source/#section-2)
    * When it asks you to select components, ONLY select one of the following, 32- or 64-bit to match your build preference:
        * Qt > Qt 5.6.1 > **msvc2013 32-bit**
        * Qt > Qt 5.6.1 > **msvc2013 64-bit**

* Download the offline installer, 32- or 64-bit to match your build preference:
    * [32-bit](http://download.qt.io/official_releases/qt/5.6/5.6.1-1/qt-opensource-windows-x86-msvc2013-5.6.1-1.exe)
    * [64-bit](http://download.qt.io/official_releases/qt/5.6/5.6.1-1/qt-opensource-windows-x86-msvc2013_64-5.6.1-1.exe)

Once Qt is installed, you need to manually configure the following:
* Set the QT_CMAKE_PREFIX_PATH environment variable to your `Qt\5.6.1\msvc2013\lib\cmake` or `Qt\5.6.1\msvc2013_64\lib\cmake` directory.
  * You can set an environment variable from Control Panel > System > Advanced System Settings > Environment Variables > New

###External Libraries

All libraries should be 32- or 64-bit to match your build preference.

CMake will need to know where the headers and libraries for required external dependencies are.

We use CMake's `fixup_bundle` to find the DLLs all of our executable targets require, and then copy them beside the executable in a post-build step. If `fixup_bundle` is having problems finding a DLL, you can fix it manually on your end by adding the folder containing that DLL to your path. Let us know which DLL CMake had trouble finding, as it is possible a tweak to our CMake files is required.

The recommended route for CMake to find the external dependencies is to place all of the dependencies in one folder and set one ENV variable - HIFI_LIB_DIR. That ENV variable should point to a directory with the following structure:

    root_lib_dir
        -> openssl
            -> bin
            -> include
            -> lib

For many of the external libraries where precompiled binaries are readily available you should be able to simply copy the extracted folder that you get from the download links provided at the top of the guide. Otherwise you may need to build from source and install the built product to this directory. The `root_lib_dir` in the above example can be wherever you choose on your system - as long as the environment variable HIFI_LIB_DIR is set to it. From here on, whenever you see %HIFI_LIB_DIR% you should substitute the directory that you chose.

####OpenSSL

Qt will use OpenSSL if it's available, but it doesn't install it, so you must install it separately.

Your system may already have several versions of the OpenSSL DLL's (ssleay32.dll, libeay32.dll) lying around, but they may be the wrong version. If these DLL's are in the PATH then QT will try to use them, and if they're the wrong version then you will see the following errors in the console:

    QSslSocket: cannot resolve TLSv1_1_client_method
    QSslSocket: cannot resolve TLSv1_2_client_method
    QSslSocket: cannot resolve TLSv1_1_server_method
    QSslSocket: cannot resolve TLSv1_2_server_method
    QSslSocket: cannot resolve SSL_select_next_proto
    QSslSocket: cannot resolve SSL_CTX_set_next_proto_select_cb
    QSslSocket: cannot resolve SSL_get0_next_proto_negotiated

To prevent these problems, install OpenSSL yourself. Download one of the following binary packages [from this website](http://slproweb.com/products/Win32OpenSSL.html):
* Win32 OpenSSL v1.0.1q
* Win64 OpenSSL v1.0.1q

Install OpenSSL into the Windows system directory, to make sure that Qt uses the version that you've just installed, and not some other version.

###Build High Fidelity using Visual Studio
Follow the same build steps from the CMake section of [BUILD.md](BUILD.md), but pass a different generator to CMake.

For 32-bit builds:

    cmake .. -G "Visual Studio 12"

For 64-bit builds:

    cmake .. -G "Visual Studio 12 Win64"

Open %HIFI_DIR%\build\hifi.sln and compile.

###Running Interface
If you need to debug Interface, you can run interface from within Visual Studio (see the section below). You can also run Interface by launching it from command line or File Explorer from %HIFI_DIR%\build\interface\Debug\interface.exe

###Debugging Interface
* In the Solution Explorer, right click interface and click Set as StartUp Project
* Set the "Working Directory" for the Interface debugging sessions to the Debug output directory so that your application can load resources. Do this: right click interface and click Properties, choose Debugging from Configuration Properties, set Working Directory to .\Debug
* Now you can run and debug interface through Visual Studio

For better performance when running debug builds, set the environment variable ```_NO_DEBUG_HEAP``` to ```1```

http://preshing.com/20110717/the-windows-heap-is-slow-when-launched-from-the-debugger/
