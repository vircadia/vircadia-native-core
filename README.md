interface
=========

Test platform for various render and interface tests for next-gen VR system.

CMake
=====

This project uses CMake to generate build files and project files for your platform.

Create a build directory in the root of your checkout and then run the CMake build from there. This will keep the rest of the directory clean, and makes the gitignore a little easier to handle (since we can just ignore build).

    mkdir build
    cd build
    cmake .. -GXCode

Those are the commands used on OS X to run CMake from the build folder and generate XCode project files.