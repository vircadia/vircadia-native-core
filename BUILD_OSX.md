Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only OS X specific instructions are found in this file.

###Homebrew
[Homebrew](http://brew.sh/) is an excellent package manager for OS X. It makes install of all High Fidelity dependencies very simple.

    brew tap highfidelity/homebrew-formulas
    brew install cmake openssl
    brew install highfidelity/formulas/qt5
    brew link qt5 --force

We have a [homebrew formulas repository](https://github.com/highfidelity/homebrew-formulas) that you can use/tap to install some of the dependencies. In the code block above qt5 is installed from a formula in this repository.

*Our [qt5 homebrew formula](https://raw.github.com/highfidelity/homebrew-formulas/master/qt5.rb) is for a patched version of Qt 5.4.x stable that removes wireless network scanning that can reduce real-time audio performance. We recommended you use this formula to install Qt.*

###Qt

Assuming you've installed Qt 5 using the homebrew instructions above, you'll need to set QT_CMAKE_PREFIX_PATH so CMake can find your installation of Qt. For Qt 5.4.1 installed via homebrew, set QT_CMAKE_PREFIX_PATH as follows.

    export QT_CMAKE_PREFIX_PATH=/usr/local/Cellar/qt5/5.4.1/lib/cmake

###Xcode
If Xcode is your editor of choice, you can ask CMake to generate Xcode project files instead of Unix Makefiles.

    cmake .. -GXcode

After running cmake, you will have the make files or Xcode project file necessary to build all of the components. Open the hifi.xcodeproj file, choose ALL_BUILD from the Product > Scheme menu (or target drop down), and click Run.

If the build completes successfully, you will have built targets for all components located in the `build/${target_name}/Debug` directories.
