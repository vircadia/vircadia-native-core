# Build OSX

*Last Updated on July 13, 2020*

Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only macOS specific instructions are found in this document.

### Homebrew

[Homebrew](https://brew.sh/) is an excellent package manager for macOS. It makes install of some Vircadia dependencies very simple.

    brew install cmake openssl npm

### Python 3

Download an install Python 3.6.6 or higher from [here](https://www.python.org/downloads/).  
Execute the `Update Shell Profile.command` script that is provided with the installer.

### OSX SDK

You will need version `10.12` of the OSX SDK for building. You can get that from [here](https://github.com/phracker/MacOSX-SDKs). You must copy it in to your Xcode SDK directory, e.g.

    cp -rp ~/Downloads/MacOSX10.12.sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/

### OpenSSL

Assuming you've installed OpenSSL using the homebrew instructions above, you'll need to set OPENSSL_ROOT_DIR so CMake can find your installations.  
For OpenSSL installed via homebrew, set OPENSSL_ROOT_DIR via
    `export OPENSSL_ROOT_DIR=/usr/local/opt/openssl`
    or by appending `-DOPENSSL_ROOT_DIR=/usr/local/opt/openssl` to `cmake`

### Xcode

You can ask CMake to generate Xcode project files instead of Unix Makefiles using the `-G Xcode` parameter after CMake. You will need to select the Xcode installation in the terminal first if you have not done so already.
    
    sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer
    
    sudo cmake ../ -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.12 -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOSX_SDK=10.12  ..

If `cmake` complains about Python 3 being missing, you may need to update your CMake binary with command `brew upgrade cmake`, or by downloading and running the latest CMake installer, depending on how you originally instaled CMake 

After running CMake, you will have the make files or Xcode project file necessary to build all of the components. Open the hifi.xcodeproj file, choose ALL_BUILD from the Product > Scheme menu (or target drop down), and click Run.

If the build completes successfully, you will have built targets for all components located in the `build/${target_name}/Debug` directories.

### make

If you build with make rather than Xcode, you can append `-j4` for assigning more threads. The number indicates the number of threads, e.g. 4.

To package the installation, you can simply run `make package` afterwards.