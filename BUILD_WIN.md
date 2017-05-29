This is a stand-alone guide for creating your first High Fidelity build for Windows 64-bit.

###Step 1. Installing Visual Studio 2013

If you don't already have the Community or Professional edition of Visual Studio 2013, download and install [Visual Studio Community 2013](https://www.visualstudio.com/en-us/news/releasenotes/vs2013-community-vs). You do not need to install any of the optional components when going through the installer.

Note: Newer versions of Visual Studio are not yet compatible. 

###Step 2. Installing CMake

Download and install the [CMake 3.8.0 win64-x64 Installer](https://cmake.org/files/v3.8/cmake-3.8.0-win64-x64.msi). Make sure "Add CMake to system PATH for all users" is checked when going through the installer.

###Step 3. Installing Qt

Download and install the [Qt 5.6.2 for Windows 64-bit (VS 2013)](http://download.qt.io/official_releases/qt/5.6/5.6.2/qt-opensource-windows-x86-msvc2013_64-5.6.2.exe). 

Keep the default components checked when going through the installer.

###Step 4. Setting Qt Environment Variable

Go to "Control Panel > System > Advanced System Settings > Environment Variables > New..." (or search “Environment Variables” in Start Search).
* Set "Variable name": QT_CMAKE_PREFIX_PATH
* Set "Variable value": `%QT_DIR%\5.6\msvc2013_64\lib\cmake`

###Step 5. Installing OpenSSL

Download and install the [Win64 OpenSSL v1.0.2k Installer](https://slproweb.com/download/Win64OpenSSL-1_0_2k.exe).

###Step 6. Running CMake to Generate Build Files

Run Command Prompt from Start and run the following commands:
    cd "%HIFI_DIR%"
    mkdir build
    cd build
    cmake .. -G "Visual Studio 12 Win64"
    
Where %HIFI_DIR% is the directory for the highfidelity repository.     

###Step 7. Making a Build

Open '%HIFI_DIR%\build\hifi.sln' using Visual Studio.

Change the Solution Configuration (next to the green play button) from "Debug" to "Release" for best performance.

Run Build > Build Solution.

###Step 8. Testing Interface

Create another environment variable (see Step #4)
* Set "Variable name": _NO_DEBUG_HEAP
* Set "Variable value": 1

In Visual Studio, right+click "interface" under the Apps folder in Solution Explorer and select "Set as Startup Project". Run Debug > Start Debugging.

Now, you should have a full build of High Fidelity and be able to run the Interface using Visual Studio. Please check our [Docs](https://wiki.highfidelity.com/wiki/Main_Page) for more information regarding the programming workflow.

Note: You can also run Interface by launching it from command line or File Explorer from %HIFI_DIR%\build\interface\Release\interface.exe

###Troubleshooting

For any problems after Step #6, first try this: 
* Delete your locally cloned copy of the highfidelity repository
* Restart your computer
* Redownload the [repository](https://github.com/highfidelity/hifi) 
* Restart directions from Step #6

####CMake gives you the same error message repeatedly after the build fails

Remove `CMakeCache.txt` found in the '%HIFI_DIR%\build' directory

####nmake cannot be found

Make sure nmake.exe is located at the following path:
    C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin
    
If not, add the directory where nmake is located to the PATH environment variable.

####Qt is throwing an error

Make sure you have the correct version (5.6.2) installed and 'QT_CMAKE_PREFIX_PATH' environment variable is set correctly.

