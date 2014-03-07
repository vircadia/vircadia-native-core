
Instructions for adding the Sixense driver to Interface
Andrzej Kapolka, November 18, 2013

NOTE: Without doing step 2, you will crash at program start time.

1. Copy the Sixense sdk folders (lib, include) into the interface/external/Sixense folder. This readme.txt should be there as well.

2. IMPORTANT: Copy the file interface/external/Sixense/lib/osx_x64/release_dll/libsixense_x64.dylib to /usr/lib

3. Delete your build directory, run cmake and build, and you should be all set.
