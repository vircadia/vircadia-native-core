# Build Windows

*Last Updated on May 17, 2020*

This is a stand-alone guide for creating your first Vircadia build for Windows 64-bit.  

Note: We are now using Visual Studio 2017 or 2019 and Qt 5.12.3.  
If you are upgrading from previous versions, do a clean uninstall of those versions before going through this guide.  

Note: The prerequisites will require about 10 GB of space on your drive. You will also need a system with at least 8GB of main memory.

### Step 1. Visual Studio & Python 3.x

If you don’t have Community or Professional edition of Visual Studio, download [Visual Studio Community 2019](https://visualstudio.microsoft.com/vs/). If you have Visual Studio 2017, you are not required to download Visual Studio 2019.

When selecting components, check "Desktop development with C++". On the right on the Summary toolbar, select the following components.

#### If you're installing Visual Studio 2017,

* Windows 8.1 SDK and UCRT SDK
* VC++ 2015.3 v14.00 (v140) toolset for desktop

#### If you're installing Visual Studio 2019,

* MSVC v141 - VS 2017 C++ x64/x86 build tools
* MSVC v140 - VS 2015 C++ build tools (v14.00)

If you do not already have a Python 3.x development environment installed, also check "Python Development" in this screen.

If you already have Visual Studio installed and need to add Python, open the "Add or remove programs" control panel and find the "Microsoft Visual Studio Installer".  Select it and click "Modify".  In the installer, select "Modify" again, then check "Python Development" and allow the installer to apply the changes.

### Step 1a.  Alternate Python

If you do not wish to use the Python installation bundled with Visual Studio, you can download the installer from [here](https://www.python.org/downloads/).  Ensure you get version 3.6.6 or higher.

### Step 2. Python Dependencies

In a command-line that can access Python's pip you will need to run the following command: 

`pip install distro`

### Step 3. Installing CMake

Download and install the latest version of CMake 3.15. 
 * Note that earlier versions of CMake will work, but there is a specific bug related to the interaction of Visual Studio 2019 and CMake versions prior to 3.15 that will cause Visual Studio to rebuild far more than it needs to on every build

Download the file named win64-x64 Installer from the [CMake Website](https://cmake.org/download/). You can access the installer on this [3.15 Version page](https://cmake.org/files/v3.15/). During installation, make sure to check "Add CMake to system PATH for all users" when prompted.

### Step 4. Create VCPKG environment variable
In the next step, you will use CMake to build Vircadia. By default, the CMake process builds dependency files in Windows' `%TEMP%` directory, which is periodically cleared by the operating system. To prevent you from having to re-build the dependencies in the event that Windows clears that directory, we recommend that you create a `HIFI_VCPKG_BASE` environment variable linked to a directory somewhere on your machine. That directory will contain all dependency files until you manually remove them.

To create this variable:
* Naviagte to 'Edit the System Environment Variables' Through the start menu.
* Click on 'Environment Variables'
* Select 'New' 
* Set "Variable name" to `HIFI_VCPKG_BASE`
* Set "Variable value" to any directory that you have control over.

Additionally, if you have Visual Studio 2019 installed and _only_ Visual Studio 2019 (i.e. you do not have Visual Studio 2017 installed) you must add an additional environment variable `HIFI_VCPKG_BOOTSTRAP` that will fix a bug in our `vcpkg` pre-build step.

To create this variable:
* Naviagte to 'Edit the System Environment Variables' Through the start menu.
* Click on 'Environment Variables'
* Select 'New' 
* Set "Variable name" to `HIFI_VCPKG_BOOTSTRAP`
* Set "Variable value" to `1`

### Step 5. Running CMake to Generate Build Files

Run Command Prompt from Start and run the following commands:  
`cd "%VIRCADIA_DIR%"`  
`mkdir build`  
`cd build`  

#### If you're using Visual Studio 2017,
Run `cmake .. -G "Visual Studio 15 Win64"`.

#### If you're using Visual Studio 2019,
Run `cmake .. -G "Visual Studio 16 2019" -A x64`.

Where `%VIRCADIA_DIR%` is the directory for the Vircadia repository.

### Step 6. Making a Build

Open `%VIRCADIA_DIR%\build\vircadia.sln` using Visual Studio.

Change the Solution Configuration (menu ribbon under the menu bar, next to the green play button) from "Debug" to "Release" for best performance.

Run from the menu bar `Build > Build Solution`.

### Step 7. Testing Interface

Create another environment variable (see Step #3)
* Set "Variable name": `_NO_DEBUG_HEAP`
* Set "Variable value": `1`

Restart Visual Studio again.

In Visual Studio, right+click "interface" under the Apps folder in Solution Explorer and select "Set as Startup Project". Run from the menu bar `Debug > Start Debugging`.

Now, you should have a full build of Vircadia and be able to run the Interface using Visual Studio.

Note: You can also run Interface by launching it from command line or File Explorer from `%VIRCADIA_DIR%\build\interface\Release\interface.exe`

## Troubleshooting

For any problems after Step #6, first try this:  
* Delete your locally cloned copy of the Vircadia repository  
* Restart your computer  
* Redownload the [repository](https://github.com/kasenvr/project-athena)  
* Restart directions from Step #6  

#### CMake gives you the same error message repeatedly after the build fails

Remove `CMakeCache.txt` found in the `%VIRCADIA_DIR%\build` directory.

#### CMake can't find OpenSSL

Remove `CMakeCache.txt` found in the `%VIRCADIA_DIR%\build` directory.  Verify that your HIFI_VCPKG_BASE environment variable is set and pointing to the correct location. Verify that the file `${HIFI_VCPKG_BASE}/installed/x64-windows/include/openssl/ssl.h` exists.
