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
#include <JurisdictionMap.h>

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

void voxelTutorial(VoxelTree * tree) {
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
}

int main(int argc, const char * argv[])
{
    qInstallMessageHandler(sharedMessageHandler);

	const char* SAY_HELLO = "--sayHello";
    if (cmdOptionExists(argc, argv, SAY_HELLO)) {
    	printf("I'm just saying hello...\n");
	}
	
	
	// Handles taking and SVO and splitting it into multiple SVOs based on
	// jurisdiction details
	const char* SPLIT_SVO = "--splitSVO";
    const char* splitSVOFile = getCmdOption(argc, argv, SPLIT_SVO);
	const char* SPLIT_JURISDICTION_ROOT = "--splitJurisdictionRoot";
	const char* SPLIT_JURISDICTION_ENDNODES = "--splitJurisdictionEndNodes";
    const char* splitJurisdictionRoot = getCmdOption(argc, argv, SPLIT_JURISDICTION_ROOT);
    const char* splitJurisdictionEndNodes = getCmdOption(argc, argv, SPLIT_JURISDICTION_ENDNODES);
    if (splitSVOFile && splitJurisdictionRoot && splitJurisdictionEndNodes) {
        char outputFileName[512];

    	printf("splitSVOFile: %s Jurisdictions Root: %s EndNodes: %s\n", 
    	        splitSVOFile, splitJurisdictionRoot, splitJurisdictionEndNodes);

        VoxelTree rootSVO;

        rootSVO.readFromSVOFile(splitSVOFile);
    	JurisdictionMap jurisdiction(splitJurisdictionRoot, splitJurisdictionEndNodes);
    	
    	printf("Jurisdiction Root Octcode: ");
    	printOctalCode(jurisdiction.getRootOctalCode());

    	printf("Jurisdiction End Nodes: %d \n", jurisdiction.getEndNodeCount());
    	for (int i = 0; i < jurisdiction.getEndNodeCount(); i++) {
    	    unsigned char* endNodeCode = jurisdiction.getEndNodeOctalCode(i);
        	printf("End Node: %d ", i);
        	printOctalCode(endNodeCode);

            // get the endNode details
            VoxelPositionSize endNodeDetails;
            voxelDetailsForCode(endNodeCode, endNodeDetails);

            // Now, create a split SVO for the EndNode.
            // copy the EndNode into a temporary tree
            VoxelTree endNodeTree;

            // create a small voxels at corners of the endNode Tree, this will is a hack
            // to work around a bug in voxel server that will send Voxel not exists
            // for regions that don't contain anything even if they're not in the
            // jurisdiction of the server
            // This hack assumes the end nodes for demo dinner since it only guarantees
            // nodes in the 8 child voxels of the main root voxel 
            endNodeTree.createVoxel(0.0, 0.0, 0.0, 0.015625, 1, 1, 1, true);
            endNodeTree.createVoxel(1.0, 0.0, 0.0, 0.015625, 1, 1, 1, true);
            endNodeTree.createVoxel(0.0, 1.0, 0.0, 0.015625, 1, 1, 1, true);
            endNodeTree.createVoxel(0.0, 0.0, 1.0, 0.015625, 1, 1, 1, true);
            endNodeTree.createVoxel(1.0, 1.0, 1.0, 0.015625, 1, 1, 1, true);
            endNodeTree.createVoxel(1.0, 1.0, 0.0, 0.015625, 1, 1, 1, true);
            endNodeTree.createVoxel(0.0, 1.0, 1.0, 0.015625, 1, 1, 1, true);
            endNodeTree.createVoxel(1.0, 0.0, 1.0, 0.015625, 1, 1, 1, true);

        	// Delete the voxel for the EndNode from the temporary tree, so we can
        	// import our endNode content into it...
            endNodeTree.deleteVoxelCodeFromTree(endNodeCode, COLLAPSE_EMPTY_TREE);

            VoxelNode* endNode = rootSVO.getVoxelAt(endNodeDetails.x, 
                                                    endNodeDetails.y, 
                                                    endNodeDetails.z, 
                                                    endNodeDetails.s);

            rootSVO.copySubTreeIntoNewTree(endNode, &endNodeTree, false);

            sprintf(outputFileName, "splitENDNODE%d%s", i, splitSVOFile);
            printf("outputFile: %s\n", outputFileName);
            endNodeTree.writeToSVOFile(outputFileName);
        	
        	// Delete the voxel for the EndNode from the root tree...
            rootSVO.deleteVoxelCodeFromTree(endNodeCode, COLLAPSE_EMPTY_TREE);
            
            // create a small voxel in center of each EndNode, this will is a hack
            // to work around a bug in voxel server that will send Voxel not exists
            // for regions that don't contain anything even if they're not in the
            // jurisdiction of the server
            float x = endNodeDetails.x + endNodeDetails.s * 0.5;
            float y = endNodeDetails.y + endNodeDetails.s * 0.5;
            float z = endNodeDetails.z + endNodeDetails.s * 0.5;
            float s = endNodeDetails.s * 0.015625;
            
            rootSVO.createVoxel(x, y, z, s, 1, 1, 1, true);
            
    	}

        sprintf(outputFileName, "splitROOT%s", splitSVOFile);
    	printf("outputFile: %s\n", outputFileName);
        rootSVO.writeToSVOFile(outputFileName);
    	
    	printf("exiting now\n");
    	return 0;
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

        myTree.writeToSVOFile("voxels.svo");

    }    
    return 0;
}