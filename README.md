# HiFi Community Edition

### [Download v0.86.0 K1 (Windows 64-bit, .zip)](https://realities.dev/cdn/hifi-community/v0860-kasen-VS-release+freshstart/Packaged_Release.zip)

This repository contains the source to many of the components in our 
alpha-stage virtual world. The project embraces distributed development.  
If you find a small bug and have a fix, pull requests are welcome. 

#### Changes for **v0.86.0** consist of:

#### Added in K1 (12/3/19)

* Audio Buffer choppy audio bugfix by increasing the buffer size.
* User Activity Logger disabled, option in code to log the reports to console.
* CMakeLists.txt configured to work for Polyvox, Interface JSDocs. (may be obsolete)
* Custom Application Title.
* Entity Script Whitelist, no scripts are whitelisted by default.
* Background CMD outputs full log, instant close of application on closing of the CMD-line.

#### Added in K2 (TBD)

* QML Interface to access and save whitelist live to interface.json.
* Add "VideoDecodeStats" to .gitignore.
* Fix VCPKG SDL2 to port files from 2.0.8 to 2.0.10 to fix CMake build issues.
* Added Github link to "About High Fidelity".
* Removed environment variable requirement for "procedural shader materials".

This build has been tested on Windows 10 Pro 64-bit w/ Nvidia graphics drivers.

### Whitelist Instructions

The whitelist checks every entity-script attempting to run on your client against a list of domains, their subfolders, or the specific script URL entirely.

The Interface has the whitelist settings under "**Settings -> Entity Script Whitelist**" for you to configure live. The whitelist checks against the domains literally, so you have to be precise to ensure security and functionality. For example, the difference between "http://" and "https://" matters as those will be seen as two different domains in the eyes of the whitelist. Separate each URL by a new line.

Do not use spaces or commas in the whitelist interface, you will only separate by commas and not new lines in the environment variables.

It is recommended that you add High Fidelity's CDN URLs ahead of time to ensure general content works right off the bat: 

```
http://mpassets.highfidelity.com/
https://raw.githubusercontent.com/highfidelity/
https://hifi-content.s3.amazonaws.com/
```

You can also set the Windows environment variable "**EXTRA_WHITELIST**" with your whitelisted domains comma separated like so: "**https://kasen.io/,http://kasen.io/,https://exampledomain.com/scriptFolder/**" 

Alternatively you can make a batch file placed in the same folder as interface.exe that sets the whitelist environment variable temporarily:

```
set "EXTRA_WHITELIST=http://mpassets.highfidelity.com/,https://raw.githubusercontent.com/highfidelity/,https://hifi-content.s3.amazonaws.com/"
interface.exe
```

### How to build interface.exe

[For Windows](https://github.com/kasenvr/hifi-community/blob/kasen/core/BUILD_WIN.md)

### Boot to Metaverse: The Goal

Too many of us have our own personal combinations of High Fidelity from C++ modifications to different default scripts, all of which are lost to time as their fullest potential is never truly shared and propagated through the system.

The goal of this repo is to give a common area to PR the very best of our findings and creations so that we may effectively take each necessary step towards our common goal of living in a true metaverse.

### Why High Fidelity?

Because of all the options, it is the only starting point that is open-source, cross-platform, fully VR integrated + fully desktop integrated with an aim for quality visuals and performance. It also does us the service of providing a foundation to start from such as entity management, full body IK, etc.

WebXR offers the open-source and decentralized aspect but does not have any of the full featured starting points such as avatars, IK, etc.

Platforms like NeosVR or VRChat are unusable from go due to their fundamental closed-source and centralized nature. A metaverse to live on cannot have the keys handed over to any one entity, if any at all.

So the necessary desire is to use High Fidelity as our foundation as a community of one, of all to build a metaverse worth living in.

### Contributors

A special thanks to the contributors of the community edition.
