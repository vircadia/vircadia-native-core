###Dependencies

* [cmake](http://www.cmake.org/cmake/resources/software.html) ~> 2.8.12.2
* [Qt](http://qt-project.org/downloads) ~> 5.3.0
* [glm](http://glm.g-truc.net/0.9.5/index.html) ~> 0.9.5.2
* [OpenSSL](https://www.openssl.org/related/binaries.html) ~> 1.0.1g
  * IMPORTANT: OpenSSL 1.0.1g is critical to avoid a security vulnerability.
* [Intel Threading Building Blocks](https://www.threadingbuildingblocks.org/) ~> 4.3

### OS Specific Build Guides
* [BUILD_OSX.md](BUILD_OSX.md) - additional instructions for OS X.
* [BUILD_LINUX.md](BUILD_LINUX.md) - additional instructions for Linux.
* [BUILD_WIN.md](BUILD_WIN.md) - additional instructions for Windows.

###CMake
Hifi uses CMake to generate build files and project files for your platform.

####Qt
In order for CMake to find the Qt5 find modules, you will need to set an ENV variable pointing to your Qt installation.

For example, a Qt5 5.3.2 installation to /usr/local/qt5 would require that QT_CMAKE_PREFIX_PATH be set with the following command. This can either be entered directly into your shell session before you build or in your shell profile (e.g.: ~/.bash_profile, ~/.bashrc, ~/.zshrc - this depends on your shell and environment).

The path it needs to be set to will depend on where and how Qt5 was installed. e.g.

    export QT_CMAKE_PREFIX_PATH=/usr/local/qt/5.3.2/clang_64/lib/cmake/
    export QT_CMAKE_PREFIX_PATH=/usr/local/Cellar/qt5/5.3.2/lib/cmake
    export QT_CMAKE_PREFIX_PATH=/usr/local/opt/qt5/lib/cmake

####Generating build files
Create a build directory in the root of your checkout and then run the CMake build from there. This will keep the rest of the directory clean.

    mkdir build
    cd build
    cmake ..

####Variables
Any variables that need to be set for CMake to find dependencies can be set as ENV variables in your shell profile, or passed directly to CMake with a `-D` flag appended to the `cmake ..` command.

For example, to pass the QT_CMAKE_PREFIX_PATH variable during build file generation:

    cmake .. -DQT_CMAKE_PREFIX_PATH=/usr/local/qt/5.2.1/lib/cmake

####Finding Dependencies
You can point our [Cmake find modules](cmake/modules/) to the correct version of dependencies by setting one of the three following variables to the location of the correct version of the dependency.

In the examples below the variable $NAME would be replaced by the name of the dependency in uppercase, and $name would be replaced by the name of the dependency in lowercase (ex: OPENSSL_ROOT_DIR, openssl).

* $NAME_ROOT_DIR - pass this variable to Cmake with the -DNAME_ROOT_DIR= flag when running Cmake to generate build files
* $NAME_ROOT_DIR - set this variable in your ENV
* HIFI_LIB_DIR - set this variable in your ENV to your High Fidelity lib folder, should contain a folder '$name'

###Optional Components

####QXmpp

You can find QXmpp [here](https://github.com/qxmpp-project/qxmpp). The inclusion of the QXmpp enables text chat in the Interface client.

OS X users who tap our [homebrew formulas repository](https://github.com/highfidelity/homebrew-formulas) can install QXmpp via homebrew - `brew install highfidelity/formulas/qxmpp`.

####Devices

You can support external input/output devices such as Leap Motion, Faceplus, Faceshift, PrioVR, MIDI, Razr Hydra and more by adding each individual SDK in the visible building path. Refer to the readme file available in each device folder in [interface/external/](interface/external) for the detailed explanation of the requirements to use the device.

