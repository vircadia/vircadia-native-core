# Creating an Installer

*Last Updated on June 16, 2021*

Follow the [build guide](BUILD.md) to figure out how to build Vircadia for your platform.

During generation, CMake should produce an `install` target and a `package` target.

The `install` target will copy the Vircadia targets and their dependencies to your `CMAKE_INSTALL_PREFIX`.  
This variable is set by the `project(hifi)` command in `CMakeLists.txt` to `C:/Program Files/hifi` and stored in `build/CMakeCache.txt`

## Packaging

To produce an installer, run the `package` target. However you will want to follow the steps specific to your platform below.

### Windows

#### Prerequisites

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

1. [Node.JS and NPM](<https://nodejs.org/en/download/>)
    1.  Install version 10.15.0 LTS (or greater)
    
#### Code Signing (optional)

For code signing to work, you will need to set the `HF_PFX_FILE` and `HF_PFX_PASSPHRASE` environment variables to be present during CMake runtime and globally as we proceed to package the installer.

#### Creating the Installer
    
1.  Perform a clean cmake from a new terminal.
1.  Open the `vircadia.sln` solution with elevated (administrator) permissions on Visual Studio and select the **Release** configuration.
1.  Build the solution.
1.  Build `packaged-server-console-npm-install` (found under **hidden/Server Console**)
1.  Build `packaged-server-console` (found under **Server Console**)  
    This will add 2 folders to `build\server-console\` -  
    `server-console-win32-x64` and `x64`
1.  Build CMakeTargets->PACKAGE   
    The installer is now available in `build\_CPack_Packages\win64\NSIS`

#### Create an MSIX Package

1. Get the 'MSIX Packaging Tool' from the Windows Store.
2. Run the process to create a new MSIX package from an existing .exe or .msi installer. This process will allow you to install Vircadia with the usual installer, however it will monitor changes to the computer to replicate the functionality in the MSIX Package. Therefore, you will want to avoid doing anything else on your computer during this process.
3. Be sure to select no shortcuts and install only the Vircadia Interface.
4. When asked for "Entry" points, select only the Interface entry and not the uninstaller. This is because the MSIX package is uninstalled by Windows itself. If for some reason the uninstaller shows up anyway, you can edit the manifest to manually remove it from view even if the uninstaller is present in the package. This is necessary to uplaod to the Windows Store.
5. Once completed, you can sign the package with this application or with other tools such as 'MSIX Hero'. It must be signed with a local certificate to test, and with a proper certificate to distribute.
6. If uploading to the Windows Store, you will have to ensure all your manifest info including publisher information matches what is registered with your Microsoft Developer account for Windows. You will see these errors and the expected values when validating it.

#### FAQ

1. **Problem:** Failure to open a file. ```File: failed opening file "\FOLDERSHARE\XYZSRelease\...\Credits.rtf" Error in script "C:\TFS\XYZProject\Releases\NullsoftInstaller\XYZWin7Installer.nsi" on line 77 -- aborting creation process```
    1. **Cause:** The complete path (current directory + relative path) has to be < 260 characters to any of the relevant files.
    1. **Solution:** Move your build and packaging folder as high up in the drive as possible to prevent an overage.

### MacOS

1. Ensure you have all the prerequisites fulfilled from the [MacOS Build Guide](BUILD_OSX.md).
2. Perform a clean CMake in your build folder. e.g.
    ```bash
    BUILD_GLOBAL_SERVICES=STABLE USE_STABLE_GLOBAL_SERVICES=1 RELEASE_BUILD=PRODUCTION BUILD_NUMBER="Insert Build Identifier here e.g. short hash of your last Git commit" RELEASE_NAME="Insert Release Name Here" STABLE_BUILD=1 PRODUCTION_BUILD=1 RELEASE_NUMBER="Insert Release Version Here e.g. 1.1.0" RELEASE_TYPE=PRODUCTION cmake -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk" -DCLIENT_ONLY=1 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.12 -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOSX_SDK=10.12  ..
    ```
3. Pick a method to build and package your release.

#### Option A: Use Xcode GUI

1. Perform a Release build of ALL_BUILD
2. Perform a Release build of `packaged-server-console` 
     This will add a folder to `build\server-console\` -  
     Sandbox-darwin-x64
3. Perform a Release build of `package`
      Installer is now available in `build/_CPack_Packages/Darwin/DragNDrop`

#### Option B: Use Terminal

1. Navigate to your build folder with your terminal.
2. `make -j4`, you can change the number to match the number of threads you would like to use.
3. `make package` to create the package.
     
### Linux

#### Server

##### Ubuntu 18.04 | .deb

1. Ensure you are using an Ubuntu 18.04 system. There is no required minimum to the amount of CPU cores needed, however it's recommended that you use as many as you have available in order to have an efficient experience.
    ```text
    Recommended CPU Cores: 16
    Minimum Disk Space: 40GB
    ```
3. Get and bootstrap Vircadia Builder.
    ```bash
    git clone https://github.com/vircadia/vircadia-builder.git
    cd vircadia-builder
    ```
3. Run Vircadia Builder.
    ```bash
    ./vircadia-builder --build server
    ```
4. If Vircadia Builder needed to install dependencies and asks you to run it again then do so. Otherwise, skip to the next step.
    ```bash
    ./vircadia-builder --build server
    ```
5. Vircadia Builder will ask you to configure it to build the server. The values will be prefilled with defaults, the following steps will explain what they are and what you might want to put. *Advanced users: See [here](BUILD.md#possible-environment-variables) for possible environment variables and settings.*
6. This value is the Git repository of Vircadia. You can set this URL to your fork of the Vircadia repository if you need to.
    ```text
    Git repository: https://github.com/vircadia/vircadia/
    # OR, for example
    Git repository: https://github.com/digisomni/vircadia/
    ```
7. This value is the tag on the repository. If you would like to use a specific version of Vircadia, typically tags will be named like this: "v2021.1.0-rc"
    ```text
    Git tag: master
    # OR, for example
    Git tag: v2021.1.0-rc
    ```
8. This value is the release type. For example, the options are `production`, `pr`, or `dev`. If you are making a build for yourself and others to use then use `production`.
    ```text
    Release type: DEV
    # OR, for example we recommend you use
    Release type: PRODUCTION
    ```
9. This value is the release version. Release numbers should be in a format of `YEAR-MAJORVERSION-MINORVERSION` which might look like this: `2021.1.0`.
    ```text
    Release number: 2021.1.0
    ```
10. This value is the build number. We typically use the hash of the most recent commit on that Git tag which might look like this: `fd6973b`.
    ```text
    Build number: fd6973b
    ```
11. This value is the directory that Vircadia will get installed to. You should leave this as the default value unless you are an advanced user.
    ```text
    Installation dir: /home/ubuntu/Vircadia
    ```
12. This value is the number of CPU cores that the Vircadia Builder will use to compile the Vircadia server. By default it will use all cores available on your build server. You should leave this as the default value it gives you for your build server.
    ```text
    CPU cores to use for Vircadia: 16
    ```
13. This value is the number of CPU cores that the Vircadia Builder will use to compile Qt5 (a required component for Vircadia). By default it will use all cores available on your build server. You should leave this as the default value it gives you for your build server.
    ```text
    CPU cores to use for Qt5: 16
    ```
14. It will ask you if you would like to proceed with the specified values. If you're happy with the configuration, type `yes`, otherwise enter `no` and press enter to start over. You can press `Ctrl` + `C` simultaneously on your keyboard to exit.
15. Vircadia Builder will now run, it may take a while. See this [table](https://github.com/vircadia/vircadia-builder#how-long-does-it-take) for estimated times.
16. Navigate to the `pkg-scripts` directory.
    ```bash
    cd ../Vircadia/source/pkg-scripts/
    ```
17. Generate the .deb package. Set `DEBVERSION` to the same version you entered for the `Release number` on Vircadia Builder. Set `DEBEMAIL` and `DEBFULLNAME` to your own information to be packaged with the release. *The version cannot begin with a letter and cannot include underscores or dashes in it.*
    ```bash
    DEBVERSION="2021.1.0" DEBEMAIL="your-email@somewhere.com" DEBFULLNAME="Your Full Name" ./make-deb-server
    ```
18. If successful, the generated .deb package will be in the `pkg-scripts` folder.

##### Amazon Linux 2 | .rpm (Deprecated)

1. Ensure you are using an Amazon Linux 2 system. You will need many CPU cores to complete this process within a reasonable time. As an alternative to AWS EC2, you may use a [virtual machine](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/amazon-linux-2-virtual-machine.html). Here are the recommended specs:
    ```text
    AWS EC2 Instance Type: C5a.4xlarge
    Recommended CPU Cores: 16
    Minimum Disk Space: 40GB
    ```
2. Update the system and install dependencies.
    ```bash
    sudo yum update -y
    sudo yum install git -y
    sudo yum install rpm-build
    ```
3. Get and bootstrap Vircadia Builder.
    ```bash
    git clone https://github.com/vircadia/vircadia-builder.git
    cd vircadia-builder
    sudo ./install_amazon_linux_deps.sh
    ```
4. Run Vircadia Builder.
    ```bash
    ./vircadia-builder --build server
    ```
5. If Vircadia Builder needed to install dependencies and asks you to run it again then do so. Otherwise, skip to the next step.
    ```bash
    ./vircadia-builder --build server
    ```
6. Vircadia Builder will ask you to configure it to build the server. The values will be prefilled with defaults, the following steps will explain what they are and what you might want to put. *Advanced users: See [here](BUILD.md#possible-environment-variables) for possible environment variables and settings.*
7. This value is the Git repository of Vircadia. You can set this URL to your fork of the Vircadia repository if you need to.
    ```text
    Git repository: https://github.com/vircadia/vircadia/
    # OR, for example
    Git repository: https://github.com/digisomni/vircadia/
    ```
8. This value is the tag on the repository. If you would like to use a specific version of Vircadia, typically tags will be named like this: "v2021.1.0-rc".
    ```text
    Git tag: master
    # OR, for example
    Git tag: v2021.1.0-rc
    ```
9. This value is the release type. For example, the options are `production`, `pr`, or `dev`. If you are making a build for yourself and others to use then use `production`.
    ```text
    Release type: DEV
    # OR, for example we recommend you use
    Release type: PRODUCTION
    ```
10. This value is the release version. Release numbers typically should be in a format of `YEAR-MAJORVERSION-MINORVERSION` which might look like this: `2021.1.0`.
    ```text
    Release number: 2021.1.0
    ```
11. This value is the build number. We typically use the hash of the most recent commit on that Git tag which might look like this: `fd6973b`.
    ```text
    Build number: fd6973b
    ```
12. This value is the directory that Vircadia will get installed to. You should leave this as the default value unless you are an advanced user.
    ```text
    Installation dir: /root/Vircadia
    ```
13. This value is the number of CPU cores that the Vircadia Builder will use to compile the Vircadia server. By default it will use all cores available on your build server given you have enough memory. You should leave this as the default value it gives you for your build server.
    ```text
    CPU cores to use for Vircadia: 16
    ```
14. This value is the number of CPU cores that the Vircadia Builder will use to compile Qt5 (a required component for Vircadia). By default it will use all cores available on your build server given you have enough memory. You should leave this as the default value it gives you for your build server.
    ```text
    CPU cores to use for Qt5: 16
    ```
15. It will ask you if you would like to proceed with the specified values. If you're happy with the configuration, type `yes`, otherwise enter `no` and press enter to start over. You can press `Ctrl` + `C` simultaneously on your keyboard to exit.
16. Vircadia Builder will now run, it may take a while. See this [table](https://github.com/vircadia/vircadia-builder#how-long-does-it-take) for estimated times.
17. Navigate to the `pkg-scripts` directory.
    ```bash
    cd ../Vircadia/source/pkg-scripts/
    ```
18. Generate the .rpm package. Set `RPMVERSION` to the same version you entered for the `Release number` on Vircadia Builder. *The version cannot begin with a letter and cannot include underscores or dashes in it.*
    ```bash
    RPMVERSION="2021.1.0" ./make-rpm-server
    ```
19. If successful, the generated .rpm package will be in the `pkg-scripts` folder of the Vircadia source files.
