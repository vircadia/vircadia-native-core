
Instructions for adding the Oculus library (LibOVR) to Interface
Stephen Birarda, March 6, 2014

You can download the Oculus SDK from https://developer.oculusvr.com/ (account creation required). Interface has been tested with SDK version 0.4.1.

1. Copy the Oculus SDK folders from the LibOVR directory (Lib, Include, Src) into the interface/externals/libovr folder.
   This readme.txt should be there as well.
   
   You may optionally choose to copy the SDK folders to a location outside the repository (so you can re-use with different checkouts and different projects). 
   If so our CMake find module expects you to set the ENV variable 'HIFI_LIB_DIR' to a directory containing a subfolder 'oculus' that contains the three folders mentioned above.

   NOTE: For Windows users, you should copy libovr.lib and libovrd.lib from the \oculus\Lib\Win32\VS2010 directory to the \libovr\Lib\Win32\ directory.

2. Clear your build directory, run cmake and build, and you should be all set.

