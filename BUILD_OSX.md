Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only macOS specific instructions are found in this file.

### Homebrew

[Homebrew](https://brew.sh/) is an excellent package manager for macOS. It makes install of some High Fidelity dependencies very simple.

    brew install cmake openssl qt

### Python 3

Download an install Python 3.6.6 or higher from [here](https://www.python.org/downloads/).  Execute the `Update Shell Profile.command` script that is provided with the installer.

### OpenSSL

Assuming you've installed OpenSSL using the homebrew instructions above, you'll need to set OPENSSL_ROOT_DIR so CMake can find your installations.
For OpenSSL installed via homebrew, set OPENSSL_ROOT_DIR:

    export OPENSSL_ROOT_DIR=/usr/local/Cellar/openssl/1.0.2l

Note that this uses the version from the homebrew formula at the time of this writing, and the version in the path will likely change.

### Qt

Assuming you've installed Qt using the homebrew instructions above, you'll need to set QT_CMAKE_PREFIX_PATH so CMake can find your installations.
For Qt installed via homebrew, set QT_CMAKE_PREFIX_PATH:

    export QT_CMAKE_PREFIX_PATH=/usr/local/Cellar/qt/5.10.1/lib/cmake

Note that this uses the version from the homebrew formula at the time of this writing, and the version in the path will likely change.

### Xcode

If Xcode is your editor of choice, you can ask CMake to generate Xcode project files instead of Unix Makefiles.

    cmake .. -G Xcode

If `cmake` complains about Python 3 being missing, you may need to update your CMake binary with command `brew upgrade cmake`, or by downloading and running the latest CMake installer, depending on how you originally instaled CMake 

After running cmake, you will have the make files or Xcode project file necessary to build all of the components. Open the hifi.xcodeproj file, choose ALL_BUILD from the Product > Scheme menu (or target drop down), and click Run.

If the build completes successfully, you will have built targets for all components located in the `build/${target_name}/Debug` directories.
