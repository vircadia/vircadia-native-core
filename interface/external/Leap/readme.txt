
Instructions for adding the Leap driver to Interface
Eric Johnston, July 10, 2013

NOTE: Without doing step 2, you will crash at program start time.

1. Copy the Leap sdk folders (lib, include, etc.) into the interface/external/Leap folder. There should be a folder already there called "stub", and this read me.txt should be there as well.

2. IMPORTANT: Copy the file interface/external/Leap/lib/libc++/libLeap.dylib to /usr/lib

3. Delete your build directory, run cmake and build, and you should be all set.
