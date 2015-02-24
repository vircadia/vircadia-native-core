
Instructions for adding the Faceshift library to Interface
Stephen Birarda, July 18th, 2014

OS X users: You can also use homebrew to get the Faceshift library by tapping our repo - highfidelity/homebrew-formulas
and then calling 'brew install highfidelity/formulas/faceshift'.

You can download the Faceshift SDK from http://download.faceshift.com/faceshift-network.zip.

Create a ‘faceshift’ folder under interface/externals.    
   
You may optionally choose to place this folder in a location outside the repository (so you can re-use with different checkouts and different projects). 

If so our CMake find module expects you to set the ENV variable 'HIFI_LIB_DIR' to a directory containing a subfolder ‘faceshift’ that contains the lib and include folders. 

1. Build a Faceshift static library from the fsbinarystream.cpp file.
    Windows: Win32 console application; no precompiled header or SDL checks; no ATL or MFC headers; Project Properties, Configuration Type = Static Library (.lib).

2. Copy the library files to the ‘lib’ folder in your Faceshift folder.
    OSX: If you build a release version call it libfaceshift.a. The debug version should be called libfaceshiftd.a.
    Windows: The release and debug versions should be called faceshift.lib and faceshiftd.lib, respectively. Copy them into a ‘Win32’ folder in your ‘lib’ folder.

3. Copy the fsbinarystream.h header file from the Faceshift SDK into the ‘include’ folder in your Faceshift folder.

4. Clear your build directory, run cmake and build, and you should be all set.

