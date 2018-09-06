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
sudo apt-get install cmake
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
git checkout tags/v0.71.0
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

Running domain server:
```bash
./domain-server/domain-server
```

Running assignment client:
```bash
./assignment-client/assignment-client -n 6
```

Running interface:
```bash
./interface/interface
```

Go to localhost in the running interface.
