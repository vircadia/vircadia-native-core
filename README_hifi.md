# THIS DOCUMENT IS OUTDATED

High Fidelity (hifi) is an early-stage technology lab experimenting with Virtual Worlds and VR. 

This repository contains the source to many of the components in our 
alpha-stage virtual world. The project embraces distributed development. 
If you'd like to help, we'll pay you -- find out more at [Worklist.net](https://worklist.net). 
If you find a small bug and have a fix, pull requests are welcome. If you'd 
like to get paid for your work, make sure you report the bug via a job on 
[Worklist.net](https://worklist.net).

We're hiring! We're looking for skilled developers; 
send your resume to hiring@highfidelity.com

##### Chat with us
Come chat with us in [our Gitter](https://gitter.im/highfidelity/hifi) if you have any questions or just want to say hi!

Documentation
=========
Documentation is available at [docs.highfidelity.com](https://docs.highfidelity.com/), if something is missing, please suggest it via a new job on Worklist (add to the hifi-docs project).

There is also detailed [documentation on our coding standards](CODING_STANDARD.md).

Contributor License Agreement (CLA)
=========
Technology companies frequently receive and use code from contributors outside the company's development team. Outside code can be a tremendous resource, but it also carries responsibility. Best practice for accepting outside contributions consists of an Apache-type Contributor License Agreement (CLA). We have modeled the High Fidelity CLA after the CLA that Google presents to developers for contributions to their projects. This CLA does not transfer ownership of code, instead simply granting a non-exclusive right for High Fidelity to use the code you’ve contributed. In that regard, you should be sure you have permission if the work relates to or uses the resources of a company that you work for. You will be asked to sign our CLA when you create your first PR or when the CLA is updated. You can also [review it here](https://gist.githubusercontent.com/hifi-gustavo/fef8f06a8233d42a0040d45c3efb97a9/raw/9981827eb94f0b18666083670b6f6a02929fb402/High%2520Fidelity%2520CLA). We sincerely appreciate your contribution and efforts toward the success of the platform.

Build Instructions 
=========
All information required to build is found in the [build guide](BUILD.md).

Running Interface
===
When you launch interface, you will automatically connect to our default domain: "root.highfidelity.io".

If you don't see anything, make sure your preferences are pointing to 
root.highfidelity.io (set your domain via Cmnd+D/Cntrl+D). If you still have no luck,
it's possible our servers are down. If you're experiencing a major bug, let us know by
adding an issue to this repository. Include details about your computer and how to 
reproduce the bug in your issue. 

To move around in-world, use the arrow keys (and Shift + up/down to fly up or 
down) or W A S D, and E or C to fly up/down. All of the other possible options 
and features are available via menus in the Interface application.

Running your own servers
========
The assignment-client and domain-server are architectural components that will allow 
you to run the full stack of the virtual world.

In order to set up your own virtual world, you need to set up and run your own 
local "domain". 

The domain-server gives a number different types of assignments to the assignment-client 
for different features: audio, avatars, voxels, particles, meta-voxels and models.

Follow the instructions in the [build guide](BUILD.md) to build the various components.

From the domain-server build directory, launch a domain-server.

    ./domain-server

Then, run an assignment-client. The assignment-client uses localhost as its assignment-server 
and talks to it on port 40102 (the default domain-server port).

In a new Terminal window, run:

    ./assignment-client

Any target can be terminated with Ctrl-C (SIGINT) in the associated Terminal window.

This assignment-client will grab one assignment from the domain-server. You can tell the 
assignment-client what type you want it to be with the `-t` option. You can also run an 
assignment-client that forks off *n* assignment-clients with the `-n` option. The `-min` 
and `-max` options allow you to set a range of required assignment-clients. This allows 
you to have flexibility in the number of assignment-clients that are running. 
See `--help` for more options.

    ./assignment-client --min 6 --max 20

To test things out, you'll need to run the Interface client.

To access your local domain in Interface, open your Preferences. On OS X, this is available 
in the Interface menu. On Linux, you'll find it in the File menu. Enter "localhost" in the 
"Domain server" field.

If everything worked, you should see that you are connected to at least one server.
Nice work!
