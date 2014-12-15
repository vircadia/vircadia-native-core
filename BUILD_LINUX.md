Please read the [general build guide](BUILD.md) for information on dependencies required for all platforms. Only Linux specific instructions are found in this file. 

###Linux Specific Dependencies
* [freeglut](http://freeglut.sourceforge.net/) ~> 2.8.0
* [zLib](http://www.zlib.net/) ~> 1.2.8

In general, as long as external dependencies are placed in OS standard locations, CMake will successfully find them during its run. When possible, you may choose to install depencies from your package manager of choice, or from source.

###Qt5 Dependencies
Should you choose not to install Qt5 via a package manager that handles dependencies for you, you may be missing some Qt5 dependencies. On Ubuntu, for example, the following additional packages are required:

    libasound2 libxmu-dev libxi-dev freeglut3-dev libasound2-dev libjack-dev

###Intel Threading Building Blocks (TBB)

Install Intel TBB from your package manager of choice, or from source (available at the [TBB Website](https://www.threadingbuildingblocks.org/)).

You must run `tbbvars` so that the find module included with this project will be able to find the correct version of TBB. `tbbvars` is located in the 'bin' folder of your TBB install.