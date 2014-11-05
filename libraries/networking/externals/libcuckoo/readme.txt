
Instructions for adding the libcuckoo library to Interface
Stephen Birarda, November 5, 2014

1. Download the libcuckoo ZIP from the High Fidelity libcuckoo fork on Github.
  	https://github.com/highfidelity/libcuckoo/archive/master.zip
   
2. Copy the libcuckoo folder from inside the ZIP to libraries/networking/externals/libcuckoo/.
	
	You should have the following structure:
	
	libraries/networking/externals/libcuckoo/libcuckoo/cuckoohash_map.hh
   
3. Delete your build directory, run cmake and build, and you should be all set.
