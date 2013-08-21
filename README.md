What is Hifi?
=========

High Fidelity (hifi) is an early-stage technology
lab experimenting with Virtual Worlds and VR. 

In this repository you'll find the source to many of the components in our 
alpha-stage virtual world. The project embraces distributed development 
and if you'd like to help, we'll pay you -- find out more at Worklist.net. 
If you find a small bug and have a fix, pull requests are welcome. If you'd 
like to get paid for your work, make sure you report the bug via a job on Worklist.net.

We're hiring! We're looking for skilled developers; 
send your resume to hiring@highfidelity.io


Building Interface
=========

Interface is our OS X and Linux build-able 
client for accessing our virtual world. 

CMake
-----
Hifi uses CMake to generate build files and project files 
for your platform. You can download CMake at cmake.org

Create a build directory in the root of your checkout and then run the 
CMake build from there. This will keep the rest of the directory clean, 
and makes the gitignore a little easier to handle (since we can just ignore build).

    mkdir build
    cd build
    cmake .. -G Xcode

Those are the commands used on OS X to run CMake from the build folder 
and generate Xcode project files. If you are building on a *nix system, 
you'll run something like "cmake .." (this will depend on your exact needs)

Other dependencies & information
----
In addition to CMake, Qt 5.1 is required to build all components.

What can I build on?
We have successfully built on OS X 10.8, Ubuntu and a few other modern Linux distributions. 
A Windows build is planned for the future, but not currently in development.

I'm in-world, what can I do?
----
If you don't see anything, make sure your preferences are pointing to root.highfidelity.io, 
if you still have no luck it's possible our servers are simply down; if you're experiencing 
a major bug, let us know by suggesting a Job on Worklist.net -- make sure to include details 
about your operating system and your computer system. 

To move around in-world, use the arrow keys (and Shift + up/down to fly up or down) 
or W A S D, and E or C to fly up/down. All of the other possible options and features 
are available via menus in the Interface application.


Other components
========

voxel-server, animation-server, audio-mixer, avatar-mixer, domain-server, pairing-server 
and space-server are architectural components that will allow you to run the full stack of 
the virtual world should you choose to.


I'm ready, I want to run my own virtual world!
========

In the voxel-server/src directory you will find a README that explains 
how to setup and administer a voxel-server.

Keep in mind that, at a minimum, you must run a domain-server, voxel-server, 
audio-mixer, and avatar-mixer to have a working virtual world. 
Basic documentation for the other components is on its way.
