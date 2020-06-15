# General Build Information

*Last Updated on May 17, 2020*

### OS Specific Build Guides

* [Build Windows](BUILD_WIN.md) - complete instructions for Windows.
* [Build Linux](BUILD_LINUX.md) - additional instructions for Linux.
* [Build OSX](BUILD_OSX.md) - additional instructions for OS X.
* [Build Android](BUILD_ANDROID.md) - additional instructions for Android

### Dependencies
- [git](https://git-scm.com/downloads): >= 1.6  
- [cmake](https://cmake.org/download/):  3.9
- [Python](https://www.python.org/downloads/): 3.6 or higher
- [Node.JS](https://nodejs.org/en/): >= 12.13.1 LTS
    - Used to build the Screen Sharing executable.

### CMake External Project Dependencies

These dependencies need not be installed manually. They are automatically downloaded on the platforms where they are required.
- [Bullet Physics Engine](https://github.com/bulletphysics/bullet3/releases):  2.83  
- [glm](https://glm.g-truc.net/0.9.8/index.html):  0.9.8  
- [Oculus SDK](https://developer.oculus.com/downloads/):   1.11 (Win32) / 0.5 (Mac)  
- [OpenVR](https://github.com/ValveSoftware/openvr):   1.11.11 (Win32 only)  
- [Polyvox](http://www.volumesoffun.com/):   0.2.1  
- [QuaZip](https://sourceforge.net/projects/quazip/files/quazip/):   0.7.3  
- [SDL2](https://www.libsdl.org/download-2.0.php):   2.0.3  
- [Intel Threading Building Blocks](https://www.threadingbuildingblocks.org/):   4.3  
- [vcpkg](https://github.com/hifi-archive/vcpkg):  
- [VHACD](https://github.com/virneo/v-hacd)  
- [zlib](http://www.zlib.net/):   1.28 (Win32 only)  
- [nvtt](https://github.com/hifi-archive/nvidia-texture-tools):   2.1.1 (customized)  

The above dependencies will be downloaded, built, linked and included automatically by CMake where we require them. The CMakeLists files that handle grabbing each of the following external dependencies can be found in the [cmake/externals folder](cmake/externals). The resulting downloads, source files and binaries will be placed in the `build/ext` folder in each of the subfolders for each external project.

These are not placed in your normal build tree when doing an out of source build so that they do not need to be re-downloaded and re-compiled every time the CMake build folder is cleared. Should you want to force a re-download and re-compile of a specific external, you can simply remove that directory from the appropriate subfolder in `build/ext`. Should you want to force a re-download and re-compile of all externals, just remove the `build/ext` folder.

#### CMake

Vircadia uses CMake to generate build files and project files for your platform.

#### Qt
CMake will download Qt 5.12.3 using vcpkg.  

To override this (i.e. use an installed Qt configuration - you will need to set a QT_CMAKE_PREFIX_PATH environment variable pointing to your Qt **lib/cmake** folder.  
This can either be entered directly into your shell session before you build or in your shell profile (e.g.: ~/.bash_profile, ~/.bashrc, ~/.zshrc - this depends on your shell and environment).  The path it needs to be set to will depend on where and how Qt5 was installed. e.g.

    export QT_CMAKE_PREFIX_PATH=/usr/local/Qt5.12.3/gcc_64/lib/cmake
    export QT_CMAKE_PREFIX_PATH=/usr/local/qt/5.12.3/clang_64/lib/cmake/
    export QT_CMAKE_PREFIX_PATH=/usr/local/Cellar/qt5/5.12.3/lib/cmake
    export QT_CMAKE_PREFIX_PATH=/usr/local/opt/qt5/lib/cmake

#### VCPKG

Vircadia uses vcpkg to download and build dependencies.
You do not need to install vcpkg.

Building the dependencies can be lengthy and the resulting files will be stored in your OS temp directory.
However, those files can potentially get cleaned up by the OS, so in order to avoid this and having to redo the lengthy build step, you can set the following environment variable:

export HIFI_VCPKG_BASE=/path/to/directory

Where /path/to/directory is the path to a directory where you wish the build files to get stored.

#### Generating Build Files

##### Possible Environment Variables

    // The URL to post the dump to.
    CMAKE_BACKTRACE_URL
    // The identifying tag of the release.
    CMAKE_BACKTRACE_TOKEN
    
    // The release version.
    RELEASE_NUMBER
    // The build commit.
    BUILD_NUMBER

    // The type of release.
    RELEASE_TYPE=PRODUCTION|PR
    RELEASE_BUILD=PRODUCTION|PR
    
    // TODO: What do these do?
    PRODUCTION_BUILD=0|1
    STABLE_BUILD=0|1
    
    // TODO: What do these do?
    USE_STABLE_GLOBAL_SERVICES=1
    BUILD_GLOBAL_SERVICES=STABLE
    
##### Generate Files

Create a build directory in the root of your checkout and then run the CMake build from there. This will keep the rest of the directory clean.

    mkdir build
    cd build
    cmake ..

If CMake gives you the same error message repeatedly after the build fails, try removing `CMakeCache.txt`.

##### Generating a release/debug only vcpkg build

In order to generate a release or debug only vcpkg package, you could use the use the `VCPKG_BUILD_TYPE` define in your cmake generate command. Building a release only vcpkg can drastically decrease the total build time.

For release only vcpkg:

`cmake .. -DVCPKG_BUILD_TYPE=release`

For debug only vcpkg:

`cmake .. -DVCPKG_BUILD_TYPE=debug`

#### Variables

Any variables that need to be set for CMake to find dependencies can be set as ENV variables in your shell profile, or passed directly to CMake with a `-D` flag appended to the `cmake ..` command.

For example, to pass the QT_CMAKE_PREFIX_PATH variable (if not using the vcpkg'ed version) during build file generation:

    cmake .. -DQT_CMAKE_PREFIX_PATH=/usr/local/qt/5.12.3/lib/cmake

#### Finding Dependencies

The following applies for dependencies we do not grab via CMake ExternalProject (OpenSSL is an example), or for dependencies you have opted not to grab as a CMake ExternalProject (via -DUSE_LOCAL_$NAME=0). The list of dependencies we grab by default as external projects can be found in [the CMake External Project Dependencies section](#cmake-external-project-dependencies).

You can point our [CMake find modules](cmake/modules/) to the correct version of dependencies by setting one of the three following variables to the location of the correct version of the dependency.

In the examples below the variable $NAME would be replaced by the name of the dependency in uppercase, and $name would be replaced by the name of the dependency in lowercase (ex: OPENSSL_ROOT_DIR, openssl).

* $NAME_ROOT_DIR - pass this variable to Cmake with the -DNAME_ROOT_DIR= flag when running Cmake to generate build files
* $NAME_ROOT_DIR - set this variable in your ENV
* HIFI_LIB_DIR - set this variable in your ENV to your Vircadia lib folder, should contain a folder '$name'

### Optional Components

#### Build Options

The following build options can be used when running CMake

* BUILD_CLIENT
* BUILD_SERVER
* BUILD_TESTS
* BUILD_TOOLS

#### Developer Build Options

* USE_GLES
* DISABLE_UI

#### Devices

You can support external input/output devices such as Leap Motion, MIDI, and more by adding each individual SDK in the visible building path. Refer to the readme file available in each device folder in [interface/external/](interface/external) for the detailed explanation of the requirements to use the device.
 