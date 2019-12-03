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

The **Start - NO STEAMVR** batch file launches and sets the whitelist environment variable for you (you have to edit in your whitelisted domains), however if you are unable to launch from the batch file and have to launch interface.exe directly then you must set the Windows environment variable "**EXTRA_WHITELIST**" with your whitelisted domains comma separated like so: "**https://kasen.io/,http://kasen.io/**" 

The whitelist checks against the domains literally, so you have to be precise to ensure security and functionality. For example, the difference between "http://" and "https://" matters as those will be seen as two different domains in the eyes of the whitelist.

### Boot to Metaverse: The Goal

Too many of us have our own personal combinations of High Fidelity from C++ modifications to different default scripts, all of which are lost to time as their fullest potential is never truly shared and propagated through the system.

The goal of this repo is to give a common area to PR the very best of our findings and creations so that we may effectively take each necessary step towards our common goal of living in a true metaverse.