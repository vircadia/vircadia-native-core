
Instructions for adding the Faceshift library to Interface
Stephen Birarda, July 18th, 2014

You can download the Faceshift SDK from http://download.faceshift.com/faceshift-network.zip.

Create a ‘faceshift’ folder under interface/externals.    
   
You may optionally choose to place this folder in a location outside the repository (so you can re-use with different checkouts and different projects). 

If so our CMake find module expects you to set the ENV variable 'HIFI_LIB_DIR' to a directory containing a subfolder ‘faceshift’ that contains the lib and include folders. 

1. Build a Faceshift static library from the fsbinarystream.cpp file. If you build a release version call it libfaceshift.a. The debug version should be called libfaceshiftd.a. Place this in the ‘lib’ folder in your Faceshift folder.

2. Copy the fsbinarystream.h header file from the Faceshift SDK into the ‘include’ folder in your Faceshift folder.

3. Clear your build directory, run cmake and build, and you should be all set.

