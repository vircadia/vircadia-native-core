
Instructions for adding SMI HMD Eye Tracking to Interface on Windows
David Rowe, 27 Jul 2015.

1.  Download and install the SMI HMD Eye Tracking software from http://update.smivision.com/iViewNG-HMD.exe.

2.  Copy the SDK folders (3rdParty, include, libs) from the SDK installation folder C:\Program Files (x86)\SMI\iViewNG-HMD\SDK 
    into the interface/externals/iViewHMD folder. This readme.txt should be there as well.

   You may optionally choose to copy the SDK folders to a location outside the repository (so you can re-use with different 
   checkouts and different projects). If so, set the ENV variable "HIFI_LIB_DIR" to a directory containing a subfolder 
   "iViewHMD" that contains the folders mentioned above.

3. Clear your build directory, run cmake and build, and you should be all set.
