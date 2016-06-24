Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only OS X specific instructions are found in this file.

###Homebrew
[Homebrew](http://brew.sh/) is an excellent package manager for OS X. It makes install of all High Fidelity dependencies very simple.

    brew tap homebrew/versions
    brew install cmake openssl qt55

We no longer require install of qt5 via our [homebrew formulas repository](https://github.com/highfidelity/homebrew-formulas). Versions of Qt that are 5.5.x provide a mechanism to disable the wireless scanning we previously had a custom patch for.

###OpenSSL and Qt

Assuming you've installed OpenSSL or Qt 5 using the homebrew instructions above, you'll need to set OPENSSL_ROOT_DIR and QT_CMAKE_PREFIX_PATH so CMake can find your installations.
For OpenSSL installed via homebrew, set OPENSSL_ROOT_DIR:

    export OPENSSL_ROOT_DIR=/usr/local/Cellar/openssl/1.0.2h_1/
    
For Qt 5.5.1 installed via homebrew, set QT_CMAKE_PREFIX_PATH as follows.

    export QT_CMAKE_PREFIX_PATH=/usr/local/Cellar/qt55/5.5.1/lib/cmake

Note that these use the versions from homebrew formulae at the time of this writing, and the version in the path will likely change.

###Xcode
If Xcode is your editor of choice, you can ask CMake to generate Xcode project files instead of Unix Makefiles.

    cmake .. -GXcode

After running cmake, you will have the make files or Xcode project file necessary to build all of the components. Open the hifi.xcodeproj file, choose ALL_BUILD from the Product > Scheme menu (or target drop down), and click Run.

If the build completes successfully, you will have built targets for all components located in the `build/${target_name}/Debug` directories.
