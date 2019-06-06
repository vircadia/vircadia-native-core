## Windows Build Guide

To build the launcher we need:

- Visual Studio Community 2017.
- CMake.


### Installing Visual Studio 2017

If you don’t have Community or Professional edition of Visual Studio 2017, download [Visual Studio Community 2017](https://www.visualstudio.com/downloads/).

When selecting components, check "Desktop development with C++".  Also on the right on the Summary toolbar, check "Windows 8.1 SDK and UCRT SDK" and "VC++ 2015.3 v140 toolset (x86,x64)".

If you already have Visual Studio installed and need to add python, open the "Add or remove programs" control panel and find the "Microsoft Visual Studio Installer".

### Installing CMake

Download and install the latest version of CMake 3.9.

Download the file named win64-x64 Installer from the [CMake Website](https://cmake.org/download/). You can access the installer on this [3.9 Version page](https://cmake.org/files/v3.9/). During installation, make sure to check "Add CMake to system PATH for all users" when prompted.

### Running CMake to Generate Build Files

Run Command Prompt from Start and run the following commands:
```
cd "%LAUNCHER_DIR%"
mkdir build
cd build
cmake .. -G "Visual Studio 15 Win64" -T v140
```

Where `%LAUNCHER_DIR%` is the directory for the PC light launcher repository.
