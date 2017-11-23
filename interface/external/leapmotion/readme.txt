
Instructions for adding the Leap Motion library (LeapSDK) to Interface
Sam Cake, June 10, 2014

You can download the Leap Developer Kit from https://developer.leapmotion.com/ (account creation required).
Interface has been tested with SDK versions:
   - LeapDeveloperKit_2.0.3+17004_win
   - LeapDeveloperKit_2.0.3+17004_mac

1. Copy the LeapSDK folders from the LeapDeveloperKit installation directory (Lib, Include) into the interface/externals/leapmotion folder.
   This readme.txt should be there as well.
   
   The files needed in the folders are:
   
   include/
   - Leap.h
   - Leap.i
   - LeapMath.h
   
   lib/
     x86/
     - Leap.dll
     - Leap.lib
     - mscvcp120.dll (optional if you already have the Msdev 2012 SDK redistributable installed)
     - mscvcr120.dll (optional if you already have the Msdev 2012 SDK redistributable installed)
   - lipLeap.dylib
     libc++/
     -libLeap.dylib

   You may optionally choose to copy the SDK folders to a location outside the repository (so you can re-use with different checkouts and different projects). 
   If so our CMake find module expects you to set the ENV variable 'HIFI_LIB_DIR' to a directory containing a subfolder 'leapmotion' that contains the 2 folders mentioned above (Include, Lib).
    
2. Clear your build directory, run cmake and build, and you should be all set.
