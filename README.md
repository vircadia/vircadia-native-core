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

If Cmake throws you an error related to Qt5 it likely cannot find your Qt5 cmake modules. 
You can solve this by setting an environment variable, QT_CMAKE_PREFIX_PATH, to the location of the folder distributed
with Qt5 that contains them.

For example, a Qt5 5.1.1 installation to /usr/local/qt5 would require that QT_CMAKE_PREFIX_PATH be set with the following command. This can either be entered directly into your shell session before you build or in your shell profile (e.g.: ~/.bash_profile, ~/.bashrc, ~/.zshrc - this depends on your shell and environment).

    export QT_CMAKE_PREFIX_PATH=/usr/local/qt/5.1.1/clang_64/lib/cmake/

The path it needs to be set to will depend on where and how Qt5 was installed.

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

On a fresh Ubuntu 13.10 install, these are all the packages you need to grab and build the hifi project:
<pre>sudo apt-get install build-essential cmake git libcurl4-openssl-dev libqt5scripttools5 libqt5svg5-dev libqt5webkit5-dev libqt5location5 qtlocation5-dev qtdeclarative5-dev qtscript5-dev qtsensors5-dev qtmultimedia5-dev qtquick1-5-dev libasound2 libxmu-dev libxi-dev freeglut3-dev libasound2-dev libjack-dev</pre>

Running Interface
-----

Using Finder, locate the interface.app Application in build/interface/Debug, 
double-click the icon, and wait for interface to launch. At this point you will automatically 
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

assignment-client, animation-server, domain-server, 
pairing-server and space-server are architectural components that will allow 
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
