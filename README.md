# HiFi Community Edition

### [Download (Windows 64-bit, .zip)](https://realities.dev/cdn/hifi-community/v0860-kasen-VS-release+freshstart/Packaged_Release.zip)

#### Changes for **v0.86.0** consist of:

* Audio Buffer choppy audio bugfix by increasing the buffer size.
* User Activity Logger disabled, option in code to log the reports to console.
* CMakeLists.txt configured to work for Polyvox, Interface JSDocs. (may be obsolete)
* Custom Application Title.
* Entity Script Whitelist, no scripts are whitelisted by default.
* Background CMD outputs full log, instant close of application on closing of the CMD-line.

This build has been tested on Windows 10 Pro 64-bit w/ Nvidia graphics drivers.

### Whitelist Instructions

The whitelist checks every entity-script attempting to run on your client against a list of domains, their subfolders, or the specific script URL entirely.

The **Start - NO STEAMVR** batch file launches and sets the whitelist environment variable for you (you have to edit in your whitelisted domains), however if you launch interface.exe directly then you must set the Windows environment variable "**EXTRA_WHITELIST**" with your whitelisted domains comma separated like so: "**https://kasen.io/,http://kasen.io/,https://exampledomain.com/scriptFolder/**" 

The whitelist checks against the domains literally, so you have to be precise to ensure security and functionality. For example, the difference between "http://" and "https://" matters as those will be seen as two different domains in the eyes of the whitelist.

### Boot to Metaverse: The Goal

Too many of us have our own personal combinations of High Fidelity from C++ modifications to different default scripts, all of which are lost to time as their fullest potential is never truly shared and propagated through the system.

The goal of this repo is to give a common area to PR the very best of our findings and creations so that we may effectively take each necessary step towards our common goal of living in a true metaverse.

### Why High Fidelity?

Because of all the options, it is the only starting point that is open-source, cross-platform, fully VR integrated + fully desktop integrated with an aim for quality visuals and performance. It also does us the service of providing a foundation to start from such as entity management, full body IK, etc.

WebXR offers the open-source and decentralized aspect but does not have any of the full featured starting points such as avatars, IK, etc.

Platforms like NeosVR or VRChat are unusable from go due to their fundamental closed-source and centralized nature. A metaverse to live on cannot have the keys handed over to any one entity, if any at all.

So the necessary desire is to use High Fidelity as our foundation as a community of one, of all to build a metaverse worth living in.
