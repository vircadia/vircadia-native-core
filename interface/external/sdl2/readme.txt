
Instructions for adding the SDL library (SDL2) to Interface
David Rowe, 11 Jan 2015

You can download the SDL development library from https://www.libsdl.org/. Interface has been tested with version 2.0.3.

1. Copy the include and lib folders into the interface/externals/sdl2 folder.
   This readme.txt should be there as well.
   
   You may optionally choose to copy the SDK folders to a location outside the repository (so you can re-use with different checkouts and different projects). 
   If so our CMake find module expects you to set the ENV variable 'HIFI_LIB_DIR' to a directory containing a subfolder 'sdl2' that contains the two folders mentioned above.

2. Clear your build directory, run cmake and build, and you should be all set.
