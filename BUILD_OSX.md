Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only OS X specific instructions are found in this file.

### Homebrew

[Homebrew](https://brew.sh/) is an excellent package manager for OS X. It makes install of some High Fidelity dependencies very simple.

    brew tap homebrew/versions
    brew install cmake openssl

### OpenSSL

Assuming you've installed OpenSSL using the homebrew instructions above, you'll need to set OPENSSL_ROOT_DIR so CMake can find your installations.
For OpenSSL installed via homebrew, set OPENSSL_ROOT_DIR:

    export OPENSSL_ROOT_DIR=/usr/local/Cellar/openssl/1.0.2h_1/

Note that this uses the version from the homebrew formula at the time of this writing, and the version in the path will likely change.

### Qt

Download and install the [Qt 5.6.2 for macOS](http://download.qt.io/official_releases/qt/5.6/5.6.2/qt-opensource-mac-x64-clang-5.6.2.dmg). 

Keep the default components checked when going through the installer.

Once Qt is installed, you need to manually configure the following:
* Set the QT_CMAKE_PREFIX_PATH environment variable to your `Qt5.6.2/5.6/clang_64/lib/cmake/` directory.

### Xcode

If Xcode is your editor of choice, you can ask CMake to generate Xcode project files instead of Unix Makefiles.

    cmake .. -GXcode

After running cmake, you will have the make files or Xcode project file necessary to build all of the components. Open the hifi.xcodeproj file, choose ALL_BUILD from the Product > Scheme menu (or target drop down), and click Run.

If the build completes successfully, you will have built targets for all components located in the `build/${target_name}/Debug` directories.
