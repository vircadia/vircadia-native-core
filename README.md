What is Hifi?
=========

High Fidelity (hifi) is an early-stage technology
lab experimenting with Virtual Worlds and VR. 

In this repository you'll find the source to many of the components in our 
alpha-stage virtual world. The project embraces distributed development 
and if you'd like to help, we'll pay you -- find out more at Worklist.net. 
If you find a small bug and have a fix, pull requests are welcome. If you'd 
like to get paid for your work, make sure you report the bug via a job on 
Worklist.net.

We're hiring! We're looking for skilled developers; 
send your resume to hiring@highfidelity.io


Building Interface & other High Fidelity Components
=========

Interface is our Windows, OS X, and Linux build-able client for accessing our virtual 
world. 

All information required to build is found in the [INSTALL file](INSTALL.md]).

Running Interface
===

When you launch interface, you will automatically connect to our default domain: "root.highfidelity.io".

I'm in-world, what can I do?
----
If you don't see anything, make sure your preferences are pointing to 
root.highfidelity.io, if you still have no luck it's possible our servers are 
simply down; if you're experiencing a major bug, let us know by adding an issue to this repository. 
Make sure to include details about your computer and how to reproduce the bug. 

To move around in-world, use the arrow keys (and Shift + up/down to fly up or 
down) or W A S D, and E or C to fly up/down. All of the other possible options 
and features are available via menus in the Interface application.

Other components
========

The assignment-client and domain-server are architectural components that will allow 
you to run the full stack of the virtual world should you choose to.


I want to run my own virtual world!
========

In order to set up your own virtual world, you need to set up and run your own 
local "domain". At a minimum, you must run a domain-server, voxel-server, 
audio-mixer, and avatar-mixer to have a working virtual world. The domain server gives three different types of assignments to the assignment-client: audio-mixer, avatar-mixer and voxel server.

Complete the steps above to build the system components, using the default Cmake Unix Makefiles generator. Start with an empty build directory.

    cmake ..

Then from the Terminal
window, change directory into the build directory, make the needed components, and then launch them.

First we make the targets we'll need.

    cd build
    make domain-server assignment-client

If after this step you're seeing something like the following

    make: Nothing to be done for `domain-server'.

you likely had Cmake generate Xcode project files and have not run `cmake ..` in a clean build directory. 

Then, launch the static domain-server. All of the targets will run in the foreground, so you'll either want to background it yourself or open a separate terminal window per target.

    cd domain-server && ./domain-server

Then, run an assignment-client with all three necessary components: avatar-mixer, audio-mixer, and voxel-server assignments. The assignment-client uses localhost as its assignment-server and talks to it on port 40102 (the default domain-server port).

In a new Terminal window, run:

    ./assignment-client/assignment-client -n 3

Any target can be terminated with Ctrl-C (SIGINT) in the associated Terminal window.

To test things out you'll want to run the Interface client. You can make that target with the following command:

    make interface

Then run the executable it builds, or open interface.app if you're on OS X. 

To access your local domain in Interface, open your Preferences -- on OS X this is available in the Interface menu, on Linux you'll find it in the File menu. Enter "localhost" in the "Domain server" field.

If everything worked you should see "Servers: 3" in the upper right. Nice work!

In the voxel-server/src directory you will find a README that explains in 
further detail how to setup and administer a voxel-server.
