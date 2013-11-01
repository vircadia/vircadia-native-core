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
#include <QString>
#include <QStringList>

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


void processSplitSVOFile(const char* splitSVOFile,const char* splitJurisdictionRoot,const char*  splitJurisdictionEndNodes) {
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
        const float verySmall = 0.015625;
        endNodeTree.createVoxel(0.0, 0.0, 0.0, verySmall, 1, 1, 1, true);
        endNodeTree.createVoxel(1.0, 0.0, 0.0, verySmall, 1, 1, 1, true);
        endNodeTree.createVoxel(0.0, 1.0, 0.0, verySmall, 1, 1, 1, true);
        endNodeTree.createVoxel(0.0, 0.0, 1.0, verySmall, 1, 1, 1, true);
        endNodeTree.createVoxel(1.0, 1.0, 1.0, verySmall, 1, 1, 1, true);
        endNodeTree.createVoxel(1.0, 1.0, 0.0, verySmall, 1, 1, 1, true);
        endNodeTree.createVoxel(0.0, 1.0, 1.0, verySmall, 1, 1, 1, true);
        endNodeTree.createVoxel(1.0, 0.0, 1.0, verySmall, 1, 1, 1, true);

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
        float s = endNodeDetails.s * verySmall;
    
        rootSVO.createVoxel(x, y, z, s, 1, 1, 1, true);
    
    }

    sprintf(outputFileName, "splitROOT%s", splitSVOFile);
    printf("outputFile: %s\n", outputFileName);
    rootSVO.writeToSVOFile(outputFileName);

    printf("exiting now\n");
}

class copyAndFillArgs {
public:
    VoxelTree* destinationTree;
    unsigned long outCount;
    unsigned long inCount;
    unsigned long originalCount;
    
};

bool copyAndFillOperation(VoxelNode* node, void* extraData) {
    copyAndFillArgs* args = (copyAndFillArgs*)extraData;
    char outputMessage[128];

    args->inCount++;
    int percentDone = (100*args->inCount/args->originalCount);

    // For each leaf node...
    if (node->isLeaf()) {
        // create a copy of the leaf in the copy destination
        float x = node->getCorner().x;
        float y = node->getCorner().y;
        float z = node->getCorner().z;
        float s = node->getScale();
        unsigned char red = node->getTrueColor()[RED_INDEX];
        unsigned char green = node->getTrueColor()[GREEN_INDEX];
        unsigned char blue = node->getTrueColor()[BLUE_INDEX];
        bool destructive = true;

        args->destinationTree->createVoxel(x, y, z, s, red, green, blue, destructive);
        args->outCount++;
        
        sprintf(outputMessage,"Completed: %d%% (%lu of %lu) - Creating voxel %lu at [%f,%f,%f,%f]",
            percentDone,args->inCount,args->originalCount,args->outCount,x,y,z,s);
        printf("%s",outputMessage);
        for (int b = 0; b < strlen(outputMessage); b++) {
            printf("\b");
        }
        
        // and create same sized leafs from this leaf voxel down to zero in the destination tree
        for (float yFill = y-s; yFill >= 0.0f; yFill -= s) {
            args->destinationTree->createVoxel(x, yFill, z, s, red, green, blue, destructive);

            args->outCount++;
        
        sprintf(outputMessage,"Completed: %d%% (%lu of %lu) - Creating fill voxel %lu at [%f,%f,%f,%f]",
            percentDone,args->inCount,args->originalCount,args->outCount,x,y,z,s);
            printf("%s",outputMessage);
            for (int b = 0; b < strlen(outputMessage); b++) {
                printf("\b");
            }
        }
    }
    return true;
}

void processFillSVOFile(const char* fillSVOFile) {
    char outputFileName[512];

    printf("fillSVOFile: %s\n", fillSVOFile);

    VoxelTree originalSVO(true); // reaveraging
    VoxelTree filledSVO(true); // reaveraging

    originalSVO.readFromSVOFile(fillSVOFile);
    qDebug("Nodes after loading %lu nodes\n", originalSVO.getVoxelCount());
    originalSVO.reaverageVoxelColors(originalSVO.rootNode);
    qDebug("Original Voxels reAveraged\n");
    qDebug("Nodes after reaveraging %lu nodes\n", originalSVO.getVoxelCount());
    
    copyAndFillArgs args;
    args.destinationTree = &filledSVO;
    args.inCount = 0;
    args.outCount = 0;
    args.originalCount = originalSVO.getVoxelCount();
    
    printf("Begin processing...\n");
    originalSVO.recurseTreeWithOperation(copyAndFillOperation, &args);
    printf("DONE processing...\n");

    qDebug("Original input nodes used for filling %lu nodes\n", args.originalCount);
    qDebug("Input nodes traversed during filling %lu nodes\n", args.inCount);
    qDebug("Nodes created during filling %lu nodes\n", args.outCount);
    qDebug("Nodes after filling %lu nodes\n", filledSVO.getVoxelCount());

    filledSVO.reaverageVoxelColors(filledSVO.rootNode);
    qDebug("Nodes after reaveraging %lu nodes\n", filledSVO.getVoxelCount());

    sprintf(outputFileName, "filled%s", fillSVOFile);
    printf("outputFile: %s\n", outputFileName);
    filledSVO.writeToSVOFile(outputFileName);

    printf("exiting now\n");
}


int main(int argc, const char * argv[])
{
    qInstallMessageHandler(sharedMessageHandler);
    
    
    const char* GET_OCTCODE = "--getOctCode";
    const char* octcodeParams = getCmdOption(argc, argv, GET_OCTCODE);
    if (octcodeParams) {

        QString octcodeParamsString(octcodeParams);
        QStringList octcodeParamsList = octcodeParamsString.split(QString(","));

        enum { X_AT, Y_AT, Z_AT, S_AT, EXPECTED_PARAMS };
        if (octcodeParamsList.size() == EXPECTED_PARAMS) {
            QString xStr = octcodeParamsList.at(X_AT);
            QString yStr = octcodeParamsList.at(Y_AT);
            QString zStr = octcodeParamsList.at(Z_AT);
            QString sStr = octcodeParamsList.at(S_AT);

            float x = xStr.toFloat()/TREE_SCALE; // 0.14745788574219;
            float y = yStr.toFloat()/TREE_SCALE; // 0.01502178955078;
            float z = zStr.toFloat()/TREE_SCALE; // 0.56540045166016;
            float s = sStr.toFloat()/TREE_SCALE; // 0.015625;

            qDebug() << "Get Octal Code for:\n";
            qDebug() << "    x:" << xStr << " [" << x << "] \n";
            qDebug() << "    y:" << yStr << " [" << y << "] \n";
            qDebug() << "    z:" << zStr << " [" << z << "] \n";
            qDebug() << "    s:" << sStr << " [" << s << "] \n";

            unsigned char* octalCode = pointToVoxel(x, y, z, s);
            QString octalCodeStr = octalCodeToHexString(octalCode);
            qDebug() << "octal code: " << octalCodeStr << "\n";

        } else {
            qDebug() << "Unexpected number of parameters for getOctCode\n";
        }
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
        processSplitSVOFile(splitSVOFile, splitJurisdictionRoot, splitJurisdictionEndNodes);
        return 0;
    }


    // Handles taking an SVO and filling in the empty space below the voxels to make it solid.
    const char* FILL_SVO = "--fillSVO";
    const char* fillSVOFile = getCmdOption(argc, argv, FILL_SVO);
    if (fillSVOFile) {
        processFillSVOFile(fillSVOFile);
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

void unitTest(VoxelTree * tree) {
    printf("unit tests...\n");

    // We want our corner voxels to be about 1/2 meter high, and our TREE_SCALE is in meters, so...    
    float voxelSize = 0.5f;

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
    
    tree->createVoxel(0, 0, 0, voxelSize, 255, 255 ,255);
    if (tree->getVoxelAt(0, 0, 0, voxelSize)) {
        printf("corner point 0,0,0 exists...\n");
    } else {
        printf("corner point 0,0,0 does not exists...\n");
    }

    tree->createVoxel(voxelSize, 0, 0, voxelSize, 255, 255 ,255);
    if (tree->getVoxelAt(voxelSize, 0, 0, voxelSize)) {
        printf("corner point voxelSize,0,0 exists...\n");
    } else {
        printf("corner point voxelSize,0,0 does not exists...\n");
    }

    tree->createVoxel(0, 0, voxelSize, voxelSize, 255, 255 ,255);
    if (tree->getVoxelAt(0, 0, voxelSize, voxelSize)) {
        printf("corner point 0, 0, voxelSize exists...\n");
    } else {
        printf("corner point 0, 0, voxelSize does not exists...\n");
    }

    tree->createVoxel(voxelSize, 0, voxelSize, voxelSize, 255, 255 ,255);
    if (tree->getVoxelAt(voxelSize, 0, voxelSize, voxelSize)) {
        printf("corner point voxelSize, 0, voxelSize exists...\n");
    } else {
        printf("corner point voxelSize, 0, voxelSize does not exists...\n");
    }

    printf("check root voxel exists...\n");
    if (tree->getVoxelAt(0,0,0,1.0)) {
        printf("of course it does\n");
    } else {
        printf("WTH!?!\n");
    }
    
}
