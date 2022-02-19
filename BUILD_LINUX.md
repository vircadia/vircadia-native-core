# Build Linux

*Last Updated on January 6, 2022*

Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only Linux specific instructions are found in this file.

You can use the [Overte Builder](https://github.com/overte-org/overte-builder) to build on Linux more easily. Alternatively, you can follow the manual steps below.

## Ubuntu 18.04

### Ubuntu 18.04 Server only
Add the universe repository:  
_(This is not enabled by default on the server edition.)_
```bash
sudo add-apt-repository universe
sudo apt-get update
```

### Install build tools:
-  First update the repositories:  
```bash
sudo apt-get update -y
sudo apt-get upgrade -y
```

-  Install git
```bash
sudo apt-get install git -y
```
Verify git was installed by running `git --version`.

-  Install g++
```bash
sudo apt-get install g++ -y
```
Verify g++ was installed by running `g++ --version`.

-  **Ubuntu 18.04** CMake
```bash
sudo apt-get install cmake -y
```
Verify CMake was installed by running `cmake --version`.

### Install build dependencies:
-  OpenSSL:
```bash
sudo apt-get install libssl-dev
```
Verify OpenSSL was installed by running `openssl version`.

- OpenGL:
```bash
sudo apt-get install libgl1-mesa-dev -y
```
Verify OpenGL:
  - First install mesa-utils with the command `sudo apt install mesa-utils -y`.
  - Then run `glxinfo | grep "OpenGL version"`.


### Extra dependencies to compile Interface on a server


- Install the following:
```bash
sudo apt-get -y install libpulse0 libnss3 libnspr4 libfontconfig1 libxcursor1 libxcomposite1 libxtst6 libxslt1.1
```

-  Misc dependencies:
```bash
sudo apt-get install libasound2 libxmu-dev libxi-dev freeglut3-dev libasound2-dev libjack0 libjack-dev libxrandr-dev libudev-dev libssl-dev zlib1g-dev
```

-  Install Python 3 and required packages:
```bash
sudo apt-get install python python3 python3-distro
```

-  Install Node.js as it is required to build the jsdoc documentation:
```bash
sudo apt-get install nodejs
```

### Get code and checkout the branch you need

Clone this repository:
```bash
git clone https://github.com/overte-org/overte.git
```

Then checkout the master branch with:
```bash
git checkout master
```

If you need a different branch, you can get a list of all tags with:
```bash
git fetch --tags
git tag
```

### Using a custom Qt build

Qt binaries are only provided for Ubuntu. In order to build on other distributions, a Qt5 install
needs to be provided by setting the `VIRCADIA_QT_PATH` environment variable to a directory containing
a Qt install.

### Using the system's Qt

The system's Qt can be used, if the development packages are installed, by setting the
`VIRCADIA_USE_SYSTEM_QT` environment variable. The minimum recommended version is Qt 5.15.2, which is
also the last version available in the Qt 5 branch. It is expected that Linux distributions will have
Qt 5.15.2 available for a long time.

### Architecture support

If the build is intended to be packaged for distribution, the `VIRCADIA_CPU_ARCHITECTURE`
CMake variable needs to be set to an architecture specific value.

By default, it is set to `-march=native -mtune=native`, which yields builds optimized for a particular
machine, but these builds will not work on machines lacking same CPU instructions.

For packaging, it is recommended to set it to a different value, for example `-msse3`. This will help ensure that the build will run on all reasonably modern CPUs.

Setting `VIRCADIA_CPU_ARCHITECTURE` to an empty string will use the default compiler settings and yield maximum compatibility.

### Compiling

Create the build directory:
```bash
cd overte
mkdir build
cd build
```

Prepare makefiles:
```bash
cmake ..
```

If cmake fails with a vcpkg error, then delete `~/vircadia-files/vcpkg/`.  

#### Server

To compile the Domain server:
```bash
make domain-server assignment-client
```

*Note: For a server, it is not necessary to compile the Interface.*

#### Interface

To compile the Interface client:
```bash
make interface
```

The commands above will compile with a single thread. If you have enough memory, you can decrease your build time using the `-j` flag. Since most x64 CPUs support two threads per core, this works out to CPU_COUNT*2. As an example, if you have a 2 core machine, you could use:
```bash
make -j4 interface
```

### Running the software

#### Domain server

Running Domain server:
```bash
./domain-server/domain-server
```

#### Assignment clients

Running assignment client:
```bash
./assignment-client/assignment-client -n 6
```

#### Interface

Running Interface:
```bash
./interface/interface
```

Go to "localhost" in the running Interface to visit your newly launched Domain server.

### Notes

If your goal is to set up a development environment, it is desirable to set the directory that vcpkg builds into with the `HIFI_VCPKG_BASE` environment variable.
For example, you might set `HIFI_VCPKG_BASE` to `/home/$USER/vcpkg`.

By default, vcpkg will build in the `~/vircadia-files/vcpkg/` directory.
