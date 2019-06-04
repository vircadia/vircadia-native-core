This is a stand-alone guide for creating your first High Fidelity build for Windows 64-bit.  
## Building High Fidelity
Note: We are now using Visual Studio 2017 or 2019 and Qt 5.12.3.  
If you are upgrading from previous versions, do a clean uninstall of those versions before going through this guide.  

Note: The prerequisites will require about 10 GB of space on your drive. You will also need a system with at least 8GB of main memory.

### Step 1. Visual Studio & Python

If you don’t have Community or Professional edition of Visual Studio, download [Visual Studio Community 2019](https://visualstudio.microsoft.com/vs/). If you have Visual Studio 2017, you are not required to download Visual Studio 2019.

When selecting components, check "Desktop development with C++". On the right on the Summary toolbar, select the following components.

#### If you're installing Visual Studio 2017,

* Windows 8.1 SDK and UCRT SDK
* VC++ 2015.3 v14.00 (v140) toolset for desktop

#### If you're installing Visual Studio 2019,

* MSVC v141 - VS 2017 C++ x64/x86 build tools
* MSVC v140 - VS 2015 C++ build tools (v14.00)

If you do not already have a Python development environment installed, also check "Python Development" in this screen.

If you already have Visual Studio installed and need to add Python, open the "Add or remove programs" control panel and find the "Microsoft Visual Studio Installer".  Select it and click "Modify".  In the installer, select "Modify" again, then check "Python Development" and allow the installer to apply the changes.

### Step 1a.  Alternate Python

If you do not wish to use the Python installation bundled with Visual Studio, you can download the installer from [here](https://www.python.org/downloads/).  Ensure you get version 3.6.6 or higher.

### Step 2. Installing CMake

Download and install the latest version of CMake 3.9.

Download the file named win64-x64 Installer from the [CMake Website](https://cmake.org/download/). You can access the installer on this [3.9 Version page](https://cmake.org/files/v3.9/). During installation, make sure to check "Add CMake to system PATH for all users" when prompted.

### Step 3. Create VCPKG environment variable
In the next step you will be using CMake to build hifi, but first it is recommended that you create an environment variable linked to a directory somewhere on your machine you have control over. The directory will hold all temporary files and, in the event of a failed build, will ensure that any dependencies  that generated prior to the failure will not need to be generated again.

To create this variable:
* Naviagte to 'Edit the System Environment Variables' Through the start menu.
* Click on 'Environment Variables'
* Select 'New' 
* Set "Variable name" to HIFI_VCPKG_BASE
* Set "Variable value" to any directory that you have control over.

### Step 4. Running CMake to Generate Build Files

Run Command Prompt from Start and run the following commands:  
`cd "%HIFI_DIR%"`  
`mkdir build`  
`cd build`  

#### If you're using Visual Studio 2017,
Run `cmake .. -G "Visual Studio 15 Win64"`.

#### If you're using Visual Studio 2019,
Run `cmake .. -G "Visual Studio 16 2019" -A x64`.

Where `%HIFI_DIR%` is the directory for the highfidelity repository.

### Step 5. Making a Build

Open `%HIFI_DIR%\build\hifi.sln` using Visual Studio.

Change the Solution Configuration (menu ribbon under the menu bar, next to the green play button) from "Debug" to "Release" for best performance.

Create another environment variable (see Step #3)
* Set "Variable name": `PreferredToolArchitecture`
* Set "Variable value": `x64`

Restart Visual Studio for the new variable to take affect.

Run from the menu bar `Build > Build Solution`.

### Step 6. Testing Interface

Create another environment variable (see Step #3)
* Set "Variable name": `_NO_DEBUG_HEAP`
* Set "Variable value": `1`

Restart Visual Studio again.

In Visual Studio, right+click "interface" under the Apps folder in Solution Explorer and select "Set as Startup Project". Run from the menu bar `Debug > Start Debugging`.

Now, you should have a full build of High Fidelity and be able to run the Interface using Visual Studio. Please check our [Docs](https://wiki.highfidelity.com/wiki/Main_Page) for more information regarding the programming workflow.

Note: You can also run Interface by launching it from command line or File Explorer from `%HIFI_DIR%\build\interface\Release\interface.exe`

## Troubleshooting

For any problems after Step #6, first try this:  
* Delete your locally cloned copy of the highfidelity repository  
* Restart your computer  
* Redownload the [repository](https://github.com/highfidelity/hifi)  
* Restart directions from Step #6  

#### CMake gives you the same error message repeatedly after the build fails

Remove `CMakeCache.txt` found in the `%HIFI_DIR%\build` directory.

#### CMake can't find OpenSSL

Remove `CMakeCache.txt` found in the `%HIFI_DIR%\build` directory.  Verify that your HIFI_VCPKG_BASE environment variable is set and pointing to the correct location.  Verify that the file `${HIFI_VCPKG_BASE}/installed/x64-windows/include/openssl/ssl.h` exists.
