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

Interface is our OS X and Linux build-able client for accessing our virtual 
world. 

CMake
-----
Hifi uses CMake to generate build files and project files 
for your platform. You can download CMake at cmake.org

Create a build directory in the root of your checkout and then run the 
CMake build from there. This will keep the rest of the directory clean, 
and makes the gitignore a little easier to handle (since we can just ignore 
build).

    mkdir build
    cd build
    cmake .. -G Xcode

Those are the commands used on OS X to run CMake from the build folder 
and generate Xcode project files. 

If you are building on a *nix system, 
you'll run something like "cmake ..", which uses the default Cmake generator for Unix Makefiles.

Building in XCode
-----

After running cmake, you will have the make files or Xcode project file 
necessary to build all of the components. For OS X, load Xcode, open the 
hifi.xcodeproj file, choose ALL_BUILD from the Product > Scheme menu (or target 
drop down), and click Run.

If the build completes successfully, you will have built targets for all HiFi
components located in the build/target_name/Debug directories.

Other dependencies & information
----
In addition to CMake, Qt 5.1 is required to build all components.

What can I build on?
We have successfully built on OS X 10.8, Ubuntu and a few other modern Linux 
distributions. A Windows build is planned for the future, but not currently in 
development.

Running Interface
-----

Using finder locate the interface.app Application in build/interface/Debug, 
double-click the icon, and wait for interface to launch. At this point you will 
connect to our default domain: "root.highfidelity.io".

I'm in-world, what can I do?
----
If you don't see anything, make sure your preferences are pointing to 
root.highfidelity.io, if you still have no luck it's possible our servers are 
simply down; if you're experiencing a major bug, let us know by suggesting a Job
on Worklist.net -- make sure to include details about your operating system and 
your computer system. 

To move around in-world, use the arrow keys (and Shift + up/down to fly up or 
down) or W A S D, and E or C to fly up/down. All of the other possible options 
and features are available via menus in the Interface application.


Other components
========

voxel-server, animation-server, audio-mixer, avatar-mixer, domain-server, 
pairing-server and space-server are architectural components that will allow 
you to run the full stack of the virtual world should you choose to.


I want to run my own virtual world!
========

In order to set up your own virtual world, you need to set up and run your own 
local "domain". At a minimum, you must run a domain-server, voxel-server, 
audio-mixer, and avatar-mixer to have a working virtual world. The audio-mixer and avatar-mixer are assignments given from the domain-server to any assignment-client that reports directly to it.

Complete the steps above to build the system components, using the default Cmake Unix Makefiles generator. Start with an empty build directory.

    cmake ..

Then from the terminal
window, change directory into the build directory, make the needed components, and then launch them.

First we make the targets we'll need.

    cd build
    make domain-server voxel-server assignment-client

If after this step you're seeing something like the following

    make: Nothing to be done for `domain-server'.

you likely had Cmake generate Xcode project files and have not run `cmake ..` in a clean build directory. 

Then, launch the static components - a domain-server and a voxel-server.

    (cd domain-server && ./domain-server --local > /tmp/domain-server.log 2>&1 &)
    ./voxel-server/voxel-server --local > /tmp/voxel-server.log 2>&1 &

Then we run as assignment-client with 2 forks to fulfill the avatar-mixer and audio-mixer assignments. It uses localhost as its assignment-server and talks to it on port 40102 (the default domain-server port).

    ./assignment-client/assignment-client -n 2 -a localhost -p 40102 > /tmp/assignment-client.log 2>&1 & 

To confirm that the components are running you can type the following command:

    ps ax | grep -w "domain-server\|voxel-server\|assignment-client"

You should see something like this:

    7523 s000  SN     0:00.03 ./domain-server --local
    7537 s000  SN     0:00.08 ./voxel-server/voxel-server --local
    7667 s000  SN     0:00.04 ./assignment-client/assignment-client -n 2 -a localhost -p 40102
    7676 s000  SN     0:00.03 ./assignment-client/assignment-client -n 2 -a localhost -p 40102
    7677 s000  SN     0:00.00 ./assignment-client/assignment-client -n 2 -a localhost -p 40102
    7735 s001  R+     0:00.00 grep -w domain-server\|voxel-server\|assignment-client

Determine the IP address of the machine you're running these servers on. Here's 
a handy resource that explains how to do this for different operating systems. 
http://kb.iu.edu/data/aapa.html

On Mac OS X, and many Unix systems you can use the ifconfig command. Typically, 
the following command will give you the IP address you need to use.

    ifconfig | grep inet | grep broadcast

You should get something like this:

    inet 192.168.1.104 netmask 0xffffff00 broadcast 192.168.1.255

Your IP address is the first set of numbers. In this case "192.168.1.104". You 
may now use this IP address to access your domain. If you are running a local 
DNS or other name service you should be able to access this IP address by name 
as well.

To access your local domain in Interface, open the Preferences dialog box, from 
the Interface menu, and enter the IP address of the local DNS name for the 
server computer in the "Domain" edit control.

In the voxel-server/src directory you will find a README that explains in 
further detail how to setup and administer a voxel-server.

