This is a stand-alone guide for creating your first High Fidelity build for Windows 64-bit.  
## Building High Fidelity
Note: We are now using Visual Studio 2017 and Qt 5.10.1. If you are upgrading from Visual Studio 2013 and Qt 5.6.2, do a clean uninstall of those versions before going through this guide.  

Note: The prerequisites will require about 10 GB of space on your drive. You will also need a system with at least 8GB of main memory.

### Step 1. Visual Studio 2017 & Python

If you don’t have Community or Professional edition of Visual Studio 2017, download [Visual Studio Community 2017](https://www.visualstudio.com/downloads/).

When selecting components, check "Desktop development with C++".  Also on the right on the Summary toolbar, check "Windows 8.1 SDK and UCRT SDK" and "VC++ 2015.3 v140 toolset (x86,x64)".  If you do not already have a python development environment installed, also check  "Python Development" in this screen.

If you already have Visual Studio installed and need to add python, open the "Add or remove programs" control panel and find the "Microsoft Visual Studio Installer".  Select it and click "Modify".  In the installer, select "Modify" again, then check "Python Development" and allow the installer to apply the changes.

### Step 1a.  Alternate Python

If you do not wish to use the Python installation bundled with Visual Studio, you can download the installer from [here](https://www.python.org/downloads/).  Ensure you get version 3.6.6 or higher.

### Step 2. Installing CMake

Download and install the latest version of CMake 3.9.

Download the file named win64-x64 Installer from the [CMake Website](https://cmake.org/download/). You can access the installer on this [3.9 Version page](https://cmake.org/files/v3.9/). During installation, make sure to check "Add CMake to system PATH for all users" when prompted.
### Step 5. Running CMake to Generate Build Files

Run Command Prompt from Start and run the following commands:
```
cd "%HIFI_DIR%"
mkdir build
cd build
cmake .. -G "Visual Studio 15 Win64"
```

Where `%HIFI_DIR%` is the directory for the highfidelity repository.

### Step 6. Making a Build

Open `%HIFI_DIR%\build\hifi.sln` using Visual Studio.

Change the Solution Configuration (menu ribbon under the menu bar, next to the green play button) from "Debug" to "Release" for best performance.

Run from the menu bar `Build > Build Solution`.

### Step 7. Testing Interface

Create another environment variable (see Step #4)
* Set "Variable name": `_NO_DEBUG_HEAP`
* Set "Variable value": `1`

In Visual Studio, right+click "interface" under the Apps folder in Solution Explorer and select "Set as Startup Project". Run from the menu bar `Debug > Start Debugging`.

Now, you should have a full build of High Fidelity and be able to run the Interface using Visual Studio. Please check our [Docs](https://wiki.highfidelity.com/wiki/Main_Page) for more information regarding the programming workflow.

Note: You can also run Interface by launching it from command line or File Explorer from `%HIFI_DIR%\build\interface\Release\interface.exe`

## Troubleshooting

For any problems after Step #7, first try this:
* Delete your locally cloned copy of the highfidelity repository
* Restart your computer
* Redownload the [repository](https://github.com/highfidelity/hifi)
* Restart directions from Step #7

#### CMake gives you the same error message repeatedly after the build fails

Remove `CMakeCache.txt` found in the `%HIFI_DIR%\build` directory.

#### CMake can't find OpenSSL

Remove `CMakeCache.txt` found in the `%HIFI_DIR%\build` directory.  Verify that your HIFI_VCPKG_BASE environment variable is set and pointing to the correct location.  Verify that the file `${HIFI_VCPKG_BASE}/installed/x64-windows/include/openssl/ssl.h` exists.

#### Qt is throwing an error

Make sure you have the correct version (5.10.1) installed and `QT_CMAKE_PREFIX_PATH` environment variable is set correctly.
