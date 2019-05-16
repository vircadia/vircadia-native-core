# Linux build guide

Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only Linux specific instructions are found in this file.

## Qt5 Dependencies

Should you choose not to install Qt5 via a package manager that handles dependencies for you, you may be missing some Qt5 dependencies. On Ubuntu, for example, the following additional packages are required:

    libasound2 libxmu-dev libxi-dev freeglut3-dev libasound2-dev libjack0 libjack-dev libxrandr-dev libudev-dev libssl-dev zlib1g-dev

## Ubuntu 16.04/18.04 specific build guide

### Ubuntu 18.04 only
Add the universe repository:  
_(This is not enabled by default on the server edition)_
```bash
sudo add-apt-repository universe
sudo apt-get update
```

### Prepare environment
Install Qt 5.10.1:
```bash
wget http://debian.highfidelity.com/pool/h/hi/hifiqt5.10.1_5.10.1_amd64.deb
sudo dpkg -i hifiqt5.10.1_5.10.1_amd64.deb
```

Install build dependencies:
```bash
sudo apt-get install libasound2 libxmu-dev libxi-dev freeglut3-dev libasound2-dev libjack0 libjack-dev libxrandr-dev libudev-dev libssl-dev zlib1g-dev
```

To compile interface in a server you must install:
```bash
sudo apt-get -y install libpulse0 libnss3 libnspr4 libfontconfig1 libxcursor1 libxcomposite1 libxtst6 libxslt1.1
```

Install build tools:
```bash
# For Ubuntu 18.04
sudo apt-get install cmake
```
```bash
# For Ubuntu 16.04
wget https://cmake.org/files/v3.9/cmake-3.9.5-Linux-x86_64.sh
sudo sh cmake-3.9.5-Linux-x86_64.sh --prefix=/usr/local --exclude-subdir
```

Install Python 3:
```bash
sudo apt-get install python3.6
```

Install node, required to build the jsdoc documentation
```bash
sudo apt-get install nodejs
```

### Get code and checkout the tag you need

Clone this repository:
```bash
git clone https://github.com/highfidelity/hifi.git
```

To compile a RELEASE version checkout the tag you need getting a list of all tags:
```bash
git fetch -a
git tags
```

Then checkout last tag with:
```bash
git checkout tags/v0.79.0
```

### Compiling

Create the build directory:
```bash
mkdir -p hifi/build
cd hifi/build
```

Prepare makefiles:
```bash
cmake -DQT_CMAKE_PREFIX_PATH=/usr/local/Qt5.10.1/5.10.1/gcc_64/lib/cmake ..
```

Start compilation of the server and get a cup of coffee:
```bash
make domain-server assignment-client
```

To compile interface:
```bash
make interface
```

In a server, it does not make sense to compile interface

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

##### Ubuntu 18.04 only

In Ubuntu 18.04 there is a problem related with NVidia driver library version.

It can be workarounded following these steps:

Uninstall incompatible nvtt libraries:
```bash
sudo apt-get remove libnvtt2 libnvtt-dev
```

Install libssl1.0-dev:
```bash
sudo apt-get -y install libssl1.0-dev
```

Clone castano nvidia-texture-tools:
```
git clone https://github.com/castano/nvidia-texture-tools
cd nvidia-texture-tools/
```

Make these changes in repo:
* In file **VERSION** set `2.2.1`
* In file **configure**:
  * set `build="release"`
  * set `-DNVTT_SHARED=1`

Configure, build and install:
```
./configure
make
sudo make install
```

Link compiled files:
```
sudo ln -s /usr/local/lib/libnvcore.so /usr/lib/libnvcore.so
sudo ln -s /usr/local/lib/libnvimage.so /usr/lib/libnvimage.so
sudo ln -s /usr/local/lib/libnvmath.so /usr/lib/libnvmath.so
sudo ln -s /usr/local/lib/libnvtt.so /usr/lib/libnvtt.so
```

After running this steps you can run interface:
```
interface/interface
```
