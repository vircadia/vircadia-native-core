# Linux build guide

Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only Linux specific instructions are found in this file.
## Qt5 Dependencies

Should you choose not to install Qt5 via a package manager that handles dependencies for you, you may be missing some Qt5 dependencies. On Ubuntu, for example, the following additional packages are required:

    libasound2 libxmu-dev libxi-dev freeglut3-dev libasound2-dev libjack0 libjack-dev libxrandr-dev libudev-dev libssl-dev zlib1g-dev

## Ubuntu 16.04/18.04 specific build guide
### Ubuntu 16.04 only
Add the following line to *.bash_profile*  
`export QT_QPA_FONTDIR=/usr/share/fonts/truetype/dejavu/`
### Ubuntu 18.04 server only
Add the universe repository:  
_(This is not enabled by default on the server edition)_  
`sudo add-apt-repository universe`  
`sudo apt-get update`  
#### Install build dependencies:
1.  OpenSSL  
`sudo apt-get install libssl-dev`  
Verify with `openssl version`  
1.  OpenGL  
Verify (first install mesa-utils - `sudo apt install mesa-utils -y`) by `glxinfo | grep "OpenGL version"`  
`sudo apt-get install libgl1-mesa-dev -y`  
`sudo ln -s /usr/lib/x86_64-linux-gnu/libGL.so.346.35 /usr/lib/x86_64-linux-gnu/libGL.so.1.2.0`  
#### To compile interface in a server you must install:
`sudo apt-get -y install libpulse0 libnss3 libnspr4 libfontconfig1 libxcursor1 libxcomposite1 libxtst6 libxslt1.1`  
#### Install build tools:
1.  First update the repositiories:  
`sudo apt-get update -y`  
`sudo apt-get upgrade -y`  
1.  git
`sudo apt-get install git -y`  
Verify by git --version  
1.  g++
`sudo apt-get install g++ -y`  
Verify by g++ --version  
1.  *Ubuntu 18.04* cmake
`sudo apt-get install cmake -y`  
Verify by git --version  
1. *Ubuntu 16.04* cmake  
`wget https://cmake.org/files/v3.14/cmake-3.14.2-Linux-x86_64.sh`  
`sudo sh cmake-3.14.2-Linux-x86_64.sh --prefix=/usr/local --exclude-subdir`  
### Get code and checkout the tag you need
Clone this repository:
`git clone https://github.com/highfidelity/hifi.git`  

To compile a RELEASE version checkout the tag you need getting a list of all tags:
`git fetch -a`  
`git tags`  
Then checkout last tag with:
`git checkout tags/v0.79.0`  
### Compiling

Create the build directory:
`mkdir -p hifi/build`  
`cd hifi/build`  
Prepare makefiles:
`cmake ..`  

*  If cmake fails with a vcpkg error - delete /tmp/hifi/vcpkg.  

Start compilation of the server and get a cup of coffee:
`make domain-server assignment-client`  

To compile interface:  
`make interface`  
In a server, it does not make sense to compile interface
### Running the software

#### Domain server

Running domain server:
`./domain-server/domain-server`  
#### Assignment clients

Running assignment client:
`./assignment-client/assignment-client -n 6`  
#### Interface

Running interface:
`./interface/interface`  

Go to localhost in the running interface.  

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

1.. Link compiled files:  
`sudo ln -s /usr/local/lib/libnvcore.so /usr/lib/libnvcore.so`  
`sudo ln -s /usr/local/lib/libnvimage.so /usr/lib/libnvimage.so`  
`sudo ln -s /usr/local/lib/libnvmath.so /usr/lib/libnvmath.so`  
`sudo ln -s /usr/local/lib/libnvtt.so /usr/lib/libnvtt.so`  

`.  After running this steps you can run interface:  
`interface/interface`  
