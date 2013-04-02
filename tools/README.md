tools
=========

Various command line tools for interacting with the Voxel Server and Interface. 

objtohio.php :

	USAGE: 
		php objtohio.php -i 'inputFileName' -o 'outputFileName' -m [blocks|wireframe|surface] -s [voxel size]
		
	DESCRIPTION:	
		Converts a Wavefront OBJ file into a voxel binary file suitable for loading locally into the client or
		loading on the voxel server or sending to the voxel server using sendvoxels.php
		
	NOTE:  
		Depending on the size of the model, it might take a lot of memory, so you will likely need to include
	    the -d memory_limit=4096M option
	       
	EXAMPLE:
	
		php -d memory_limit=4096M objtohio.php -i "Samples/little girl head-obj.obj" -o girl-test.hio -s 0.006


sendvoxels.php :

	USAGE:	
		sendvoxels.php -i 'inputFileName' -s [serverip] -p [port] -c [I|R]

	DESCRIPTION:	
		Sends the contents of a voxel binary file to a voxel server. Can either Insert or Remove voxels. Command defaults
		to "I" insert.

	EXAMPLES:
	
		php sendvoxels.php -s 192.168.1.116 -i 'girl-test.hio' -c I
		php sendvoxels.php -s 192.168.1.116 -i 'girl-test.hio' -c R
		php sendvoxels.php -s 192.168.1.116 -i 'girl-test.hio'


