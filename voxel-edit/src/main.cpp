//
//  main.cpp
//  Voxel Edit
//
//  Created by Brad Hefta-Gaub on 05/03/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <VoxelTree.h>
#include <SharedUtil.h>

VoxelTree myTree;

int _nodeCount=0;
bool countVoxelsOperation(VoxelNode* node, void* extraData) {
    if (node->isColored()){
        _nodeCount++;
    }
    return true; // keep going
}

void addLandscape(VoxelTree * tree) {
    printf("Adding Landscape...\n");
}

void addScene(VoxelTree * tree) {
    printf("adding scene...\n");

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

    // Now some more examples... a little more complex
    printf("creating corner points...\n");
    tree->createVoxel(0              , 0              , 0              , voxelSize, 255, 255 ,255);
    tree->createVoxel(1.0 - voxelSize, 0              , 0              , voxelSize, 255, 0   ,0  );
    tree->createVoxel(0              , 1.0 - voxelSize, 0              , voxelSize, 0  , 255 ,0  );
    tree->createVoxel(0              , 0              , 1.0 - voxelSize, voxelSize, 0  , 0   ,255);
    tree->createVoxel(1.0 - voxelSize, 0              , 1.0 - voxelSize, voxelSize, 255, 0   ,255);
    tree->createVoxel(0              , 1.0 - voxelSize, 1.0 - voxelSize, voxelSize, 0  , 255 ,255);
    tree->createVoxel(1.0 - voxelSize, 1.0 - voxelSize, 0              , voxelSize, 255, 255 ,0  );
    tree->createVoxel(1.0 - voxelSize, 1.0 - voxelSize, 1.0 - voxelSize, voxelSize, 255, 255 ,255);
    printf("DONE creating corner points...\n");

    // Now some more examples... creating some lines using the line primitive
    printf("creating voxel lines...\n");
    // We want our line voxels to be about 1/32 meter high, and our TREE_SCALE is in meters, so...    
    float lineVoxelSize = 1.f / (32 * TREE_SCALE);
    rgbColor red   = {255, 0, 0};
    rgbColor green = {0, 255, 0};
    rgbColor blue  = {0, 0, 255};
    tree->createLine(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), lineVoxelSize, blue);
    tree->createLine(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), lineVoxelSize, red);
    tree->createLine(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), lineVoxelSize, green);
    printf("DONE creating lines...\n");

    // Now some more examples... creating some spheres using the sphere primitive
    // We want the smallest unit of our spheres to be about 1/16th of a meter tall
    float sphereVoxelSize = 1.f / (16 * TREE_SCALE);
    printf("creating spheres... sphereVoxelSize=%f\n",sphereVoxelSize);
    tree->createSphere(0.25, 0.5, 0.5, 0.5, sphereVoxelSize, true, false, true);
    printf("one sphere added... sphereVoxelSize=%f\n",sphereVoxelSize);

    tree->createSphere(0.030625, 0.5, 0.5, (0.25 - 0.06125), sphereVoxelSize, true, true);
    printf("two spheres added... sphereVoxelSize=%f\n",sphereVoxelSize);
    tree->createSphere(0.030625, (0.75 - 0.030625), (0.75 - 0.030625), (0.75 - 0.06125), sphereVoxelSize, true, true);
    printf("three spheres added... sphereVoxelSize=%f\n",sphereVoxelSize);
    tree->createSphere(0.030625, (0.75 - 0.030625), (0.75 - 0.030625), 0.06125, sphereVoxelSize, true, true);
    printf("four spheres added... sphereVoxelSize=%f\n",sphereVoxelSize);
    tree->createSphere(0.030625, (0.75 - 0.030625), 0.06125, (0.75 - 0.06125), sphereVoxelSize, true, true);
    printf("five spheres added... sphereVoxelSize=%f\n",sphereVoxelSize);
    tree->createSphere(0.06125, 0.125, 0.125, (0.75 - 0.125), sphereVoxelSize, true, true);

    float radius = 0.0125f;
    printf("6 spheres added...\n");
    tree->createSphere(radius, 0.25, radius * 5.0f, 0.25, (1.0 / 4096), true, true);
    printf("7 spheres added...\n");
    tree->createSphere(radius, 0.125, radius * 5.0f, 0.25, (1.0 / 4096), true, true);
    printf("8 spheres added...\n");
    tree->createSphere(radius, 0.075, radius * 5.0f, 0.25, (1.0 / 4096), true, true);
    printf("9 spheres added...\n");
    tree->createSphere(radius, 0.05, radius * 5.0f, 0.25, (1.0 / 4096), true, true);
    printf("10 spheres added...\n");
    tree->createSphere(radius, 0.025, radius * 5.0f, 0.25, (1.0 / 4096), true, true);
    printf("11 spheres added...\n");
    printf("DONE creating spheres...\n");
    // Here's an example of how to recurse the tree and do some operation on the nodes as you recurse them.
    // This one is really simple, it just couts them...
    // Look at the function countVoxelsOperation() for an example of how you could use this function
    _nodeCount=0;
    tree->recurseTreeWithOperation(countVoxelsOperation);
    printf("Nodes after adding scene %d nodes\n", _nodeCount);

    printf("DONE adding scene of spheres...\n");
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
        addScene(&myTree);
        myTree.writeToFileV2("voxels.hio2");
    }    
    return 0;
}