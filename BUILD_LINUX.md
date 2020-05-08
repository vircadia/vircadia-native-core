# Build Linux

*Last Updated on January 20, 2020*

Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only Linux specific instructions are found in this file.

## Ubuntu 16.04/18.04 specific build guide
### Ubuntu 16.04 only
Add the following line to *.bash_profile*  
`export QT_QPA_FONTDIR=/usr/share/fonts/truetype/dejavu/`
### Ubuntu 18.04 server only
Add the universe repository:  
_(This is not enabled by default on the server edition)_
```bash
sudo add-apt-repository universe
sudo apt-get update
```
#### Install build tools:
-  First update the repositories:  
```bash
sudo apt-get update -y
sudo apt-get upgrade -y
```
-  git
```bash
sudo apt-get install git -y
```
Verify by git --version  
-  g++
```bash
sudo apt-get install g++ -y
```
Verify by g++ --version  
-  *Ubuntu 18.04* cmake
```bash
sudo apt-get install cmake -y
```
Verify by cmake --version  
- *Ubuntu 16.04* cmake  
```bash
wget https://cmake.org/files/v3.14/cmake-3.14.2-Linux-x86_64.sh
sudo sh cmake-3.14.2-Linux-x86_64.sh --prefix=/usr/local --exclude-subdir
```
#### Install build dependencies:
-  OpenSSL:
```bash
sudo apt-get install libssl-dev
```
Verify with `openssl version`  
- OpenGL:
```bash
sudo apt-get install libgl1-mesa-dev -y
sudo ln -s /usr/lib/x86_64-linux-gnu/libGL.so.346.35 /usr/lib/x86_64-linux-gnu/libGL.so.1.2.0
```
- Verify OpenGL:
  - First install mesa-utils with the command `sudo apt install mesa-utils -y`
  - Then run `glxinfo | grep "OpenGL version"`  
#### To compile interface in a server you must install:
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
-  Install node, required to build the jsdoc documentation:
```bash
sudo apt-get install nodejs
```

### Get code and checkout the branch you need

Clone this repository:
```bash
git clone https://github.com/kasenvr/project-athena.git
```

To compile a DEV version checkout the branch you need. To get a list of all tags:
```bash
git fetch -a
```

Then checkout the main branch with:
```bash
git checkout kasen/core
```

### Using a custom Qt build

Qt binaries are only provided for Ubuntu. In order to build on other distributions, a Qt5 install needs to be provided as follows:

* Set `VIRCADIA_USE_PREBUILT_QT=1`
* Set `VIRCADIA_USE_QT_VERSION` to the Qt version (defaults to `5.12.3`)
* Set `HIFI_QT_BASE=/path/to/qt`

Qt must be installed in `$HIFI_QT_BASE/$VIRCADIA_USE_QT_VERSION/qt5-install`.

### Compiling

Create the build directory:
```bash
cd project-athena
mkdir build
cd build
```

Prepare makefiles:
```bash
cmake ..
```

- If cmake fails with a vcpkg error - delete /tmp/hifi/vcpkg.  

Start compilation of the server and get a cup of coffee:
```bash
make domain-server assignment-client
```

To compile interface:
```bash
make interface
```

The commands above will compile with a single thread. If you have enough memory,
you can decrease your build time using the `-j` flag. Since most x64 CPUs
support two threads per core, this works out to CPU_COUNT*2. As an example, if
you have a 2 core machine, you could use:
```
make -j4 interface
```

In a server, it does not make sense to compile interface.

### Running the software

#### Domain server

Running domain server:
```bash
./domain-server/domain-server
```

#### Assignment clients

Running assignment client:
```bash
./assignment-client/assignment-client -n 6
```

#### Interface

Running interface:
```bash
./interface/interface
```

Go to localhost in the running interface.

#### Notes

If your goal is to set up a development environment, it is desirable to set the
directory that vcpkg builds into with the `HIFI_VCPKG_BASE` environment variable.
For example, you might set `HIFI_VCPKG_BASE` to `/home/$USER/vcpkg`.
By default, vcpkg will build in the system `/tmp` directory.

##### Ubuntu 18.04 only

In Ubuntu 18.04 there is a problem related with NVidia driver library version.

It can be worked around following these steps:

1.  Uninstall incompatible nvtt libraries:  
`sudo apt-get remove libnvtt2 libnvtt-dev`  

1.  Install libssl1.0-dev:  
`sudo apt-get -y install libssl1.0-dev`  

1.  Clone castano nvidia-texture-tools:  
`git clone https://github.com/castano/nvidia-texture-tools`  
`cd nvidia-texture-tools/` 

1.  Make these changes in repo:  
* In file **VERSION** set `2.2.1`  
* In file **configure**:  
  * set `build="release"`  
  * set `-DNVTT_SHARED=1`  

1.  Configure, build and install:  
`./configure`  
`make`  
`sudo make install`  

1. Link compiled files:  
`sudo ln -s /usr/local/lib/libnvcore.so /usr/lib/libnvcore.so`  
`sudo ln -s /usr/local/lib/libnvimage.so /usr/lib/libnvimage.so`  
`sudo ln -s /usr/local/lib/libnvmath.so /usr/lib/libnvmath.so`  
`sudo ln -s /usr/local/lib/libnvtt.so /usr/lib/libnvtt.so`  

1.  After running these steps you can run interface:  
`interface/interface`  
