
Instructions for adding the PrioVR driver to Interface
Andrzej Kapolka, May 12, 2014

1. Download and install the YEI drivers from https://www.yeitechnology.com/yei-3-space-sensor-software-suite.  If using
   Window 8+, follow the workaround instructions at http://forum.yeitechnology.com/viewtopic.php?f=3&t=24.
   
2. Get the PrioVR skeleton API, open ts_c_api2_priovr2/visual_studio/ThreeSpace_API_2/ThreeSpace_API_2.sln
   in Visual Studio, and build it.

3. Copy ts_c_api2_priovr2/visual_studio/ThreeSpace_API_2/Skeletal_API/yei_skeletal_api.h to interface/external/priovr/include,
   ts_c_api2_priovr2/visual_studio/ThreeSpace_API_2/Debug/Skeletal_API.lib to interface/external/priovr/lib, and
   ts_c_api2_priovr2/visual_studio/ThreeSpace_API_2/Debug/*.dll to your path.
   
4. Delete your build directory, run cmake and build, and you should be all set.

