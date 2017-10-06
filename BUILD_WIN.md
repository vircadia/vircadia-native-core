This is a stand-alone guide for creating your first High Fidelity build for Windows 64-bit.

## Building High Fidelity
Note: We are now using Visual Studio 2017 and Qt 5.9.1. If you are upgrading from Visual Studio 2013 and Qt 5.6.2, do a clean uninstall of those versions before going through this guide. 

Note: The prerequisites will require about 10 GB of space on your drive. You will also need a system with at least 8GB of main memory.

### Step 1. Visual Studio 2017

If you don’t have Community or Professional edition of Visual Studio 2017, download [Visual Studio Community 2017](https://www.visualstudio.com/downloads/). 

When selecting components, check "Desktop development with C++." Also check "Windows 8.1 SDK and UCRT SDK" and "VC++ 2015.3 v140 toolset (x86,x64)" on the Summary toolbar on the right.

### Step 2. Installing CMake

Download and install the latest version of CMake 3.9. Download the file named  win64-x64 Installer from the [CMake Website](https://cmake.org/download/). Make sure to check "Add CMake to system PATH for all users" when prompted during installation.

### Step 3. Installing Qt

Download and install the [Qt Online Installer](https://www.qt.io/download-open-source/?hsCtaTracking=f977210e-de67-475f-a32b-65cec207fd03%7Cd62710cd-e1db-46aa-8d4d-2f1c1ffdacea). While installing, you only need to have the following components checked under Qt 5.9.1: "msvc2017 64-bit", "Qt WebEngine", and "Qt Script (Deprecated)".

Note: Installing the Sources is optional but recommended if you have room for them (~2GB). 

### Step 4. Setting Qt Environment Variable

Go to `Control Panel > System > Advanced System Settings > Environment Variables > New...` (or search “Environment Variables” in Start Search).
* Set "Variable name": `QT_CMAKE_PREFIX_PATH`
* Set "Variable value": `C:\Qt\5.9.1\msvc2017_64\lib\cmake` 

### Step 5. Installing [vcpkg](https://github.com/Microsoft/vcpkg)

 * Clone the VCPKG [repository](https://github.com/Microsoft/vcpkg)
 * Follow the instructions in the [readme](https://github.com/Microsoft/vcpkg/blob/master/README.md) to bootstrap vcpkg
   * Note, you may need to do these in a _Developer Command Prompt_
 * Set an environment variable VCPKG_ROOT to the location of the cloned repository
   * Close and re-open any command prompts after setting the environment variable so that they will pick up the change

### Step 6. Installing OpenSSL via vcpkg

 * In the vcpkg directory, install the 64 bit OpenSSL package with the command `vcpkg install openssl:x64-windows`
 * Once the build completes you should have a file `ssl.h` in `${VCPKG_ROOT}/installed/x64-windows/include/openssl`
  
### Step 7. Running CMake to Generate Build Files

Run Command Prompt from Start and run the following commands:
```
cd "%HIFI_DIR%"
mkdir build
cd build
cmake .. -G "Visual Studio 15 Win64"
```
    
Where `%HIFI_DIR%` is the directory for the highfidelity repository.     

### Step 8. Making a Build

Open `%HIFI_DIR%\build\hifi.sln` using Visual Studio.

Change the Solution Configuration (next to the green play button) from "Debug" to "Release" for best performance.

Run `Build > Build Solution`.

### Step 9. Testing Interface

Create another environment variable (see Step #4)
* Set "Variable name": `_NO_DEBUG_HEAP`
* Set "Variable value": `1`

In Visual Studio, right+click "interface" under the Apps folder in Solution Explorer and select "Set as Startup Project". Run `Debug > Start Debugging`.

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

Remove `CMakeCache.txt` found in the `%HIFI_DIR%\build` directory.  Verify that your VCPKG_ROOT environment variable is set and pointing to the correct location.  Verify that the file `${VCPKG_ROOT}/installed/x64-windows/include/openssl/ssl.h` exists.

#### Qt is throwing an error

Make sure you have the correct version (5.9.1) installed and `QT_CMAKE_PREFIX_PATH` environment variable is set correctly.
