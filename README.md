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
and generate Xcode project files. If you are building on a *nix system, 
you'll run something like "cmake .." (this will depend on your exact needs)

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
audio-mixer, and avatar-mixer to have a working virtual world. 

Complete the steps above to build the system components. Then from the terminal
window, change directory into the build direction, then launch the following 
components.

    ./domain-server/Debug/domain-server --local &
    ./voxel-server/Debug/voxel-server --local &
    ./avatar-mixer/Debug/avatar-mixer --local &
    ./audio-mixer/Debug/audio-mixer --local &

To confirm that the components are running you can type the following command:

    ps ax | grep -w "domain-server\|voxel-server\|audio-mixer\|avatar-mixer"

You should see something like this:

    70488 s001  S      0:00.04 ./domain-server/Debug/domain-server --local
    70489 s001  S      0:00.23 ./voxel-server/Debug/voxel-server --local
    70490 s001  S      0:00.03 ./avatar-mixer/Debug/avatar-mixer --local
    70491 s001  S      0:00.48 ./audio-mixer/Debug/audio-mixer --local
    70511 s001  S+     0:00.00 grep -w domain-server\|voxel-server\|audio-mixer\
                              |avatar-mixer

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

