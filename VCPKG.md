[VCPKG](https://github.com/Microsoft/vcpkg) is an open source package management system created by Microsoft, intially just for Windows based system, but eventually extended to cover Linux and OSX as well, and in theory extensible enough to cover additional operating systems.

VCPKG is now our primary mechanism for managing the external libraries and tools on which we rely to build our applications.

Conventional usage of VCPKG involves cloning the repository, running the bootstrapping script to build the vcpkg binary, and then calling the binary to install a set of libraries.  The libraries themselves are specified by a set of port files inside the [repository](https://github.com/Microsoft/vcpkg/tree/master/ports)

Because the main VCPKG repository does not contain all the ports we want, and because we want to be able to manage the precise versions of our dependencies, rather than allow it to be outside of our control, instead of using the main vcpkg repository, we use a combination of a [fork](https://github.com/highfidelity/vcpkg) of the repository (which allows us to customize the vcpkg binary, currently necessary to deal with some out of date tools on our build hosts) and a set of [custom port files](./cmake/ports) stored in our own repository.

## Adding new packages to vcpkg

Note...  Android vcpkg usage is still experimental.  Contact Austin for more detailed information if you need to add a new package for use by Android.  

### Setup development environment

In order to add new packages, you will need to set up an environment for testing.  This assumes you already have the tools for normal Hifi development (git, cmake, a working C++ compiler, etc)

* Clone our vcpkg [fork](https://github.com/highfidelity/vcpkg)
* Remove the ports directory from the checkout and symlink to our own [custom port files](./cmake/ports)
* Bootstrap the vcpkg binary with the `bootstrap-vcpkg.sh` or `bootstrap-vcpkg.bat` script

### Add a new port skeleton

Your new package will require, at minimum, a `CONTROL` file and a `portfile.cmake` file, located in a subdirectory of the ports folder.  Assuming you're creating a new dependency named `foo` it should be located in `ports/foo` under the vcpkg directory.  The `CONTROL` file will contain a small number of fields, such as the name, version, description and any other vcpkg ports on which you depend.  The `portfile.cmake` is a CMake script that will instruct vcpkg how to build the packages.  We'll cover that in more depth in a moment.  For now, just create one and leave it blank.

### Add a reference to your package to one or more of the hifi meta-packages

We have three meta-packages used to do our building.  When you modify one of these packages, make sure to bump the version number in the `CONTROL` file for the package

#### hifi-deps

This metapackage contains anything required for building the server or shared components.  For instance, the `glm`, `tbb` and `zlib` packages are declared here because they're used everywhere, not just in our client application code. 

#### hifi-client-deps

This metapackage contains anything required for building the client.  For example, `sdl2` is listed here because it's required for our joystick input, but not for the server or shared components.  Note that `hifi-client-deps` depends on `hifi-deps`, so you don't have to declare something twice if it's used in both he server and client.  Just declare it in `hifi-deps` and it will still be includeded transitively.

#### hifi-host-tools

This metapackage contains anything we use to create executables that will then be used in the build process.  The `hifi-deps` and `hifi-client-deps` packages are built for the target architecture, which may be different than the host architecture (for instance, when building for Android).  The `hifi-host-tools` packages are always build for the host architecture, because they're tools that are intended to be run as part of the build process.  Scribe for example is used at build time to generate shaders.  Building an arm64 version of Scribe is useless because we need to run it on the host machine.

Note that packages can appear in both the `hifi-host-tools` and one of the other metapackages, indicating that the package both contains a library which we will use at runtime, and a tool which we will use at build time.  The `spirv-tools` package is an example.

### Implement the portfile.cmake

How the portfile is written depends on what kind of package you're working with.  It's basically still a CMake script, but there are a number of [functions](https://vcpkg.readthedocs.io/en/latest/maintainers/portfile-functions/) available to make fetching and building packages easier.

Typically there are three areas you need to deal with

* Getting the source
* Building the source
* Installing the artifacts

#### Getting sources

Getting sources from github, gitlab or bitbucket is easy.  There are special functions specifcially for those.  See the [etc2comp portfile](./cmake/ports/etc2comp/portfile.cmake) for an example of fetching source via github.

If the project isn't available that way, you can still use the [vcpkg_download_distfile](https://vcpkg.readthedocs.io/en/latest/maintainers/vcpkg_download_distfile/) function to explicitly download an archive and then use [vcpkg_extract_source_archive](https://vcpkg.readthedocs.io/en/latest/maintainers/vcpkg_extract_source_archive/) to unpack it.  See the [zlib portfile](./cmake/ports/zlib/portfile.cmake) for an example there.

#### Building

If your package uses CMake, you'll be able to use the [vcpkg_configure_cmake](https://vcpkg.readthedocs.io/en/latest/maintainers/vcpkg_configure_cmake/) and [vcpkg_build_cmake](https://vcpkg.readthedocs.io/en/latest/maintainers/vcpkg_build_cmake/) commands to configure and build the package.  If you're going to be relying on the CMake installation functionality, you can just call [vcpkg_install_cmake](https://vcpkg.readthedocs.io/en/latest/maintainers/vcpkg_install_cmake/), since it will implicitly run the build before the install.

If your package is not binary, but doesn't use CMake, you're just going to have to figure it out.

If your package is binary, then you can just skip this step

#### Installing

Once you've built the package, you need to install the artifacts in the target directory.  Ideally, your package's CMake INSTALL commands will do the right thing.  However, there are usually some things you have to do manually.  Since VCPKG will build both the release and debug versions for all packages, you need to make sure if your package installed headers that you remove the _debug_ versions of these headers.  This is typically done with the `file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)`.  Additionally, if your package creates any standalone executables, you need to make sure they're installed in the destination `tools` directory, not the `bin` or `lib` directories, which are specifically for shared library binaries (like .so or .dll files) and link library files (like .a or .lib files) respectively.

If you're dealing with a binary package, then you'll need to explicitly perform all the required copies from the location where you extracted the archive to the installation directory.  An example of this is available in the [openssl-android portfile](./cmake/ports/openssl-android/portfile.cmake)

### Commit and test

Once you've tested building your new package locally, you'll need to commit and push the changes and additions to the portfiles you've made and then monitor the build hosts to verify that the new package successfully built on all the target environments.
