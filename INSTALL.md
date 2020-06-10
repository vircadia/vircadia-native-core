# Creating an Installer

Follow the [build guide](BUILD.md) to figure out how to build Vircadia for your platform.

During generation, CMake should produce an `install` target and a `package` target.

The `install` target will copy the Vircadia targets and their dependencies to your `CMAKE_INSTALL_PREFIX`.  
This variable is set by the `project(hifi)` command in `CMakeLists.txt` to `C:/Program Files/hifi` and stored in `build/CMakeCache.txt`

### Packaging

To produce an installer, run the `package` target.

#### Windows

To produce an executable installer on Windows, the following are required:

1. [7-zip](<https://www.7-zip.org/download.html>)  

1. [Nullsoft Scriptable Install System](http://nsis.sourceforge.net/Download) - 3.04  
  Install using defaults (will install to `C:\Program Files (x86)\NSIS`)
1. [UAC Plug-in for Nullsoft](http://nsis.sourceforge.net/UAC_plug-in) - 0.2.4c  
    1. Extract Zip
    1. Copy `UAC.nsh` to `C:\Program Files (x86)\NSIS\Include\`
    1. Copy `Plugins\x86-ansi\UAC.dll` to `C:\Program Files (x86)\NSIS\Plugins\x86-ansi\`
    1. Copy `Plugins\x86-unicode\UAC.dll` to `C:\Program Files (x86)\NSIS\Plugins\x86-unicode\`
1. [nsProcess Plug-in for Nullsoft](http://nsis.sourceforge.net/NsProcess_plugin) - 1.6 (use the link marked **nsProcess_1_6.7z**)
    1. Extract Zip
    1. Copy `Include\nsProcess.nsh` to `C:\Program Files (x86)\NSIS\Include\`
    1. Copy `Plugins\nsProcess.dll` to `C:\Program Files (x86)\NSIS\Plugins\x86-ansi\`
    1. Copy `Plugins\nsProcessW.dll` to `C:\Program Files (x86)\NSIS\Plugins\x86-unicode\`

1. [InetC Plug-in for Nullsoft](http://nsis.sourceforge.net/Inetc_plug-in) - 1.0
    1. Extract Zip
    1. Copy `Plugin\x86-ansi\InetC.dll` to `C:\Program Files (x86)\NSIS\Plugins\x86-ansi\`
    1. Copy `Plugin\x86-unicode\InetC.dll` to `C:\Program Files (x86)\NSIS\Plugins\x86-unicode\`
    
1. [NSISpcre Plug-in for Nullsoft](http://nsis.sourceforge.net/NSISpcre_plug-in) - 1.0
    1. Extract Zip
    1. Copy `NSISpre.nsh` to `C:\Program Files (x86)\NSIS\Include\`
    1. Copy `NSISpre.dll` to `C:\Program Files (x86)\NSIS\Plugins\x86-ansi\`

1. [nsisSlideshow Plug-in for Nullsoft](<http://wiz0u.free.fr/prog/nsisSlideshow/>) - 1.7
   1.  Extract Zip
   1.  Copy `bin\nsisSlideshow.dll` to `C:\Program Files (x86)\NSIS\Plugins\x86-ansi\`
   1.  Copy `bin\nsisSlideshowW.dll` to `C:\Program Files (x86)\NSIS\Plugins\x86-unicode\`

1. [Nsisunz plug-in for Nullsoft](http://nsis.sourceforge.net/Nsisunz_plug-in)
   1.  Download both Zips and unzip
   1.  Copy `nsisunz\Release\nsisunz.dll` to `C:\Program Files (x86)\NSIS\Plugins\x86-ansi\`
   1.  Copy `NSISunzU\Plugin unicode\nsisunz.dll` to `C:\Program Files (x86)\NSIS\Plugins\x86-unicode\`

1. [ApplicationID plug-in for Nullsoft]() - 1.0
   1.  Download [`Pre-built DLLs`](<https://github.com/connectiblutz/NSIS-ApplicationID/releases/download/1.1/NSIS-ApplicationID.zip>)
   1.  Extract Zip
   1.  Copy `Release\ApplicationID.dll` to `C:\Program Files (x86)\NSIS\Plugins\x86-ansi\`
   1.  Copy `ReleaseUnicode\ApplicationID.dll` to `C:\Program Files (x86)\NSIS\Plugins\x86-unicode\`

1. [Node.JS and NPM](<https://www.npmjs.com/get-npm>)
    1.  Install version 10.15.0 LTS
    
1.  Perform a clean cmake from a new terminal.
1.  Open the `vircadia.sln` solution with elevated (administrator) permissions on Visual Studio and select the **Release** configuration.
1.  Build the solution.
1.  Build CMakeTargets->INSTALL
1.  Build `packaged-server-console-npm-install` (found under **hidden/Server Console**)
1.  Build `packaged-server-console` (found under **Server Console**)  
    This will add 2 folders to `build\server-console\` -  
    `server-console-win32-x64` and `x64`
1.  Build CMakeTargets->PACKAGE   
    The installer is now available in `build\_CPack_Packages\win64\NSIS`

#### OS X
1.   [npm](<https://www.npmjs.com/get-npm>)
      Install version 12.16.3 LTS
   
1.  Perform a clean CMake.
1.  Perform a Release build of ALL_BUILD
1.  Perform a Release build of `packaged-server-console` 
     This will add a folder to `build\server-console\` -  
     Sandbox-darwin-x64
1.  Perform a Release build of `package`
      Installer is now available in `build/_CPack_Packages/Darwin/DragNDrop
      
### FAQ

1. **Problem:** Failure to open a file. ```File: failed opening file "\FOLDERSHARE\XYZSRelease\...\Credits.rtf" Error in script "C:\TFS\XYZProject\Releases\NullsoftInstaller\XYZWin7Installer.nsi" on line 77 -- aborting creation process```
    1. **Cause:** The complete path (current directory + relative path) has to be < 260 characters to any of the relevant files.
    1. **Solution:** Move your build and packaging folder as high up in the drive as possible to prevent an overage.
