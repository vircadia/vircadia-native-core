Follow the [build guide](BUILD.md) to figure out how to build High Fidelity for your platform.

During generation, CMake should produce an `install` target and a `package` target.

### Install

The `install` target will copy the High Fidelity targets and their dependencies to your `CMAKE_INSTALL_PREFIX`.

### Packaging

To produce an installer, run the `package` target.

#### Windows

To produce an executable installer on Windows, the following are required:

- [Nullsoft Scriptable Install System](http://nsis.sourceforge.net/Download) - 3.0b3
- [UAC Plug-in for Nullsoft](http://nsis.sourceforge.net/UAC_plug-in) - 0.2.4c
- [nsProcess Plug-in for Nullsoft](http://nsis.sourceforge.net/NsProcess_plugin) - 1.6
- [Inetc Plug-in for Nullsoft](http://nsis.sourceforge.net/Inetc_plug-in) - 1.0
- [NSISpcre Plug-in for Nullsoft](http://nsis.sourceforge.net/NSISpcre_plug-in) - 1.0
- [nsisSlideshow Plug-in for Nullsoft](http://nsis.sourceforge.net/NsisSlideshow_plug-in) - 1.7
- [Nsisunz plug-in for Nullsoft](http://nsis.sourceforge.net/Nsisunz_plug-in)

Run the `package` target to create an executable installer using the Nullsoft Scriptable Install System.

#### OS X

Run the `package` target to create an Apple Disk Image (.dmg).
