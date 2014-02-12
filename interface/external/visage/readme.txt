
Instructions for adding the Visage driver to Interface
Andrzej Kapolka, February 11, 2014

1. Copy the Visage sdk folders (lib, include, dependencies) into the interface/external/visage folder.
   This readme.txt should be there as well.

2. Copy the Visage configuration data folder (visageSDK-MacOS/Samples/MacOSX/data/) to interface/resources/visage
   (i.e., so that interface/resources/visage/candide3.wfm is accessible)

3. Copy the Visage license file to interface/resources/visage/license.vlc.

4. Delete your build directory, run cmake and build, and you should be all set.

