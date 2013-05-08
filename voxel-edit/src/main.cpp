//
//  main.cpp
//  Voxel Edit
//
//  Created by Brad Hefta-Gaub on 05/03/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <VoxelTree.h>
#include <SharedUtil.h>
#include <SceneUtils.h>

VoxelTree myTree;

void voxelTutorial(VoxelTree* tree) {
    // We want our corner voxels to be about 1/2 meter high, and our TREE_SCALE is in meters, so...    
    float voxelSize = 0.5f / TREE_SCALE;

    // Here's an example of how to create a voxel.
    printf("creating corner points...\n");
    tree->createVoxel(0, 0, 0, voxelSize, 255, 255 ,255);

    // Here's an example of how to test if a voxel exists
    VoxelNode* node = tree->getVoxelAt(0, 0, 0, voxelSize);
    if (node) {
        // and how to access it's color
        printf("corner point 0,0,0 exists... color is (%d,%d,%d) \n", 
            node->getColor()[0], node->getColor()[1], node->getColor()[2]);
    }

    // here's an example of how to delete a voxel
    printf("attempting to delete corner point 0,0,0\n");
    tree->deleteVoxelAt(0, 0, 0, voxelSize);

    // Test to see that the delete worked... it should be FALSE...
    if (tree->getVoxelAt(0, 0, 0, voxelSize)) {
        printf("corner point 0,0,0 exists...\n");
    } else {
        printf("corner point 0,0,0 does not exists...\n");
    }
}

int main(int argc, const char * argv[])
{
	const char* SAY_HELLO = "--sayHello";
    if (cmdOptionExists(argc, argv, SAY_HELLO)) {
    	printf("I'm just saying hello...\n");
	}

	const char* DONT_CREATE_FILE = "--dontCreateSceneFile";
    bool dontCreateFile = cmdOptionExists(argc, argv, DONT_CREATE_FILE);
    
    if (dontCreateFile) {
    	printf("You asked us not to create a scene file, so we will not.\n");
	} else {
    	printf("Creating Scene File...\n");
    	
        const char* RUN_TUTORIAL = "--runTutorial";
        if (cmdOptionExists(argc, argv, RUN_TUTORIAL)) {
            voxelTutorial(&myTree);
        }

        const char* ADD_CORNERS_AND_AXIS_LINES = "--addCornersAndAxisLines";
        if (cmdOptionExists(argc, argv, ADD_CORNERS_AND_AXIS_LINES)) {
            addCornersAndAxisLines(&myTree);
        }

        const char* ADD_SPHERE_SCENE = "--addSphereScene";
        if (cmdOptionExists(argc, argv, ADD_SPHERE_SCENE)) {
            addSphereScene(&myTree);
        }

        const char* ADD_SURFACE_SCENE = "--addSurfaceScene";
        if (cmdOptionExists(argc, argv, ADD_SURFACE_SCENE)) {
            addSurfaceScene(&myTree);
        }

        unsigned long nodeCount = myTree.getVoxelCount();
        printf("Nodes after adding scenes: %ld nodes\n", nodeCount);

        myTree.writeToFileV2("voxels.hio2");

    }    
    return 0;
}