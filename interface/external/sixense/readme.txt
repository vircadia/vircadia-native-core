
Instructions for adding the Sixense driver to Interface
Andrzej Kapolka, November 18, 2013

1. Copy the Sixense sdk folders (bin, include, lib, and samples) into the interface/external/Sixense folder. This readme.txt should be there as well.
    
   You may optionally choose to copy the SDK folders to a location outside the repository (so you can re-use with different checkouts and different projects).
   If so our CMake find module expects you to set the ENV variable 'HIFI_LIB_DIR' to a directory containing a subfolder 'sixense' that contains the folders mentioned above.

3. Delete your build directory, run cmake and build, and you should be all set.
