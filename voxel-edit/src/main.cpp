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


int _nodeCount=0;
bool countVoxelsOperation(VoxelTreeElement* node, void* extraData) {
    if (node->isColored()){
        _nodeCount++;
    }
    return true; // keep going
}

void addLandscape(VoxelTree * tree) {
    qDebug("Adding Landscape...");
}

void voxelTutorial(VoxelTree * tree) {
    qDebug("adding scene...");

    // We want our corner voxels to be about 1/2 meter high, and our TREE_SCALE is in meters, so...
    float voxelSize = 0.5f / TREE_SCALE;

    // Here's an example of how to create a voxel.
    qDebug("creating corner points...");
    tree->createVoxel(0, 0, 0, voxelSize, 255, 255 ,255);

    // Here's an example of how to test if a voxel exists
    VoxelTreeElement* node = tree->getVoxelAt(0, 0, 0, voxelSize);
    if (node) {
        // and how to access it's color
        qDebug("corner point 0,0,0 exists... color is (%d,%d,%d)",
            node->getColor()[0], node->getColor()[1], node->getColor()[2]);
    }

    // here's an example of how to delete a voxel
    qDebug("attempting to delete corner point 0,0,0");
    tree->deleteVoxelAt(0, 0, 0, voxelSize);

    // Test to see that the delete worked... it should be FALSE...
    if (tree->getVoxelAt(0, 0, 0, voxelSize)) {
        qDebug("corner point 0,0,0 exists...");
    } else {
        qDebug("corner point 0,0,0 does not exists...");
    }
}


void processSplitSVOFile(const char* splitSVOFile,const char* splitJurisdictionRoot,const char*  splitJurisdictionEndNodes) {
    char outputFileName[512];

    qDebug("splitSVOFile: %s Jurisdictions Root: %s EndNodes: %s",
            splitSVOFile, splitJurisdictionRoot, splitJurisdictionEndNodes);

    VoxelTree rootSVO;

    rootSVO.readFromSVOFile(splitSVOFile);
    JurisdictionMap jurisdiction(splitJurisdictionRoot, splitJurisdictionEndNodes);

    qDebug("Jurisdiction Root Octcode: ");
    printOctalCode(jurisdiction.getRootOctalCode());

    qDebug("Jurisdiction End Nodes: %d ", jurisdiction.getEndNodeCount());
    for (int i = 0; i < jurisdiction.getEndNodeCount(); i++) {
        unsigned char* endNodeCode = jurisdiction.getEndNodeOctalCode(i);
        qDebug("End Node: %d ", i);
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
        endNodeTree.deleteOctalCodeFromTree(endNodeCode, COLLAPSE_EMPTY_TREE);

        VoxelTreeElement* endNode = rootSVO.getVoxelAt(endNodeDetails.x,
                                                endNodeDetails.y,
                                                endNodeDetails.z,
                                                endNodeDetails.s);

        rootSVO.copySubTreeIntoNewTree(endNode, &endNodeTree, false);

        sprintf(outputFileName, "splitENDNODE%d%s", i, splitSVOFile);
        qDebug("outputFile: %s", outputFileName);
        endNodeTree.writeToSVOFile(outputFileName);

        // Delete the voxel for the EndNode from the root tree...
        rootSVO.deleteOctalCodeFromTree(endNodeCode, COLLAPSE_EMPTY_TREE);

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
    qDebug("outputFile: %s", outputFileName);
    rootSVO.writeToSVOFile(outputFileName);

    qDebug("exiting now");
}

class copyAndFillArgs {
public:
    VoxelTree* destinationTree;
    unsigned long outCount;
    unsigned long inCount;
    unsigned long originalCount;

};

bool copyAndFillOperation(OctreeElement* element, void* extraData) {
    VoxelTreeElement* voxel = (VoxelTreeElement*)element;
    copyAndFillArgs* args = (copyAndFillArgs*)extraData;
    char outputMessage[128];

    args->inCount++;
    int percentDone = (100*args->inCount/args->originalCount);

    // For each leaf node...
    if (voxel->isLeaf()) {
        // create a copy of the leaf in the copy destination
        float x = voxel->getCorner().x;
        float y = voxel->getCorner().y;
        float z = voxel->getCorner().z;
        float s = voxel->getScale();
        unsigned char red = voxel->getColor()[RED_INDEX];
        unsigned char green = voxel->getColor()[GREEN_INDEX];
        unsigned char blue = voxel->getColor()[BLUE_INDEX];
        bool destructive = true;

        args->destinationTree->createVoxel(x, y, z, s, red, green, blue, destructive);
        args->outCount++;

        sprintf(outputMessage,"Completed: %d%% (%lu of %lu) - Creating voxel %lu at [%f,%f,%f,%f]",
            percentDone,args->inCount,args->originalCount,args->outCount,x,y,z,s);
        printf("%s",outputMessage);
        for (unsigned int b = 0; b < strlen(outputMessage); b++) {
            printf("\b");
        }

        // and create same sized leafs from this leaf voxel down to zero in the destination tree
        for (float yFill = y-s; yFill >= 0.0f; yFill -= s) {
            args->destinationTree->createVoxel(x, yFill, z, s, red, green, blue, destructive);

            args->outCount++;

        sprintf(outputMessage,"Completed: %d%% (%lu of %lu) - Creating fill voxel %lu at [%f,%f,%f,%f]",
            percentDone,args->inCount,args->originalCount,args->outCount,x,y,z,s);
            printf("%s",outputMessage);
            for (unsigned int b = 0; b < strlen(outputMessage); b++) {
                printf("\b");
            }
        }
    }
    return true;
}

void processFillSVOFile(const char* fillSVOFile) {
    char outputFileName[512];

    qDebug("fillSVOFile: %s", fillSVOFile);

    VoxelTree originalSVO(true); // reaveraging
    VoxelTree filledSVO(true); // reaveraging

    originalSVO.readFromSVOFile(fillSVOFile);
    qDebug("Nodes after loading %lu nodes", originalSVO.getOctreeElementsCount());
    originalSVO.reaverageOctreeElements();
    qDebug("Original Voxels reAveraged");
    qDebug("Nodes after reaveraging %lu nodes", originalSVO.getOctreeElementsCount());

    copyAndFillArgs args;
    args.destinationTree = &filledSVO;
    args.inCount = 0;
    args.outCount = 0;
    args.originalCount = originalSVO.getOctreeElementsCount();
    qDebug("Begin processing...");
    originalSVO.recurseTreeWithOperation(copyAndFillOperation, &args);
    qDebug("DONE processing...");

    qDebug("Original input nodes used for filling %lu nodes", args.originalCount);
    qDebug("Input nodes traversed during filling %lu nodes", args.inCount);
    qDebug("Nodes created during filling %lu nodes", args.outCount);
    qDebug("Nodes after filling %lu nodes", filledSVO.getOctreeElementsCount());

    filledSVO.reaverageOctreeElements();
    qDebug("Nodes after reaveraging %lu nodes", filledSVO.getOctreeElementsCount());

    sprintf(outputFileName, "filled%s", fillSVOFile);
    qDebug("outputFile: %s", outputFileName);
    filledSVO.writeToSVOFile(outputFileName);

    qDebug("exiting now");
}

void unitTest(VoxelTree * tree);


int main(int argc, const char * argv[])
{
    VoxelTree myTree;

    qInstallMessageHandler(sharedMessageHandler);

    unitTest(&myTree);


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

            qDebug() << "Get Octal Code for:";
            qDebug() << "    x:" << xStr << " [" << x << "]";
            qDebug() << "    y:" << yStr << " [" << y << "]";
            qDebug() << "    z:" << zStr << " [" << z << "]";
            qDebug() << "    s:" << sStr << " [" << s << "]";

            unsigned char* octalCode = pointToVoxel(x, y, z, s);
            QString octalCodeStr = octalCodeToHexString(octalCode);
            qDebug() << "octal code: " << octalCodeStr;

        } else {
            qDebug() << "Unexpected number of parameters for getOctCode";
        }
        return 0;
    }

    const char* DECODE_OCTCODE = "--decodeOctCode";
    const char* decodeParam = getCmdOption(argc, argv, DECODE_OCTCODE);
    if (decodeParam) {

        QString decodeParamsString(decodeParam);
        unsigned char* octalCodeToDecode = hexStringToOctalCode(decodeParamsString);

        VoxelPositionSize details;
        voxelDetailsForCode(octalCodeToDecode, details);

        delete[] octalCodeToDecode;

        qDebug() << "octal code to decode: " << decodeParamsString;
        qDebug() << "Details for Octal Code:";
        qDebug() << "    x:" << details.x << "[" << details.x * TREE_SCALE << "]";
        qDebug() << "    y:" << details.y << "[" << details.y * TREE_SCALE << "]";
        qDebug() << "    z:" << details.z << "[" << details.z * TREE_SCALE << "]";
        qDebug() << "    s:" << details.s << "[" << details.s * TREE_SCALE << "]";
        return 0;
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
        qDebug("You asked us not to create a scene file, so we will not.");
    } else {
        qDebug("Creating Scene File...");

        const char* RUN_TUTORIAL = "--runTutorial";
        if (cmdOptionExists(argc, argv, RUN_TUTORIAL)) {
            voxelTutorial(&myTree);
        }

        const char* ADD_CORNERS_AND_AXIS_LINES = "--addCornersAndAxisLines";
        if (cmdOptionExists(argc, argv, ADD_CORNERS_AND_AXIS_LINES)) {
            addCornersAndAxisLines(&myTree);
        }

        const char* ADD_SURFACE_SCENE = "--addSurfaceScene";
        if (cmdOptionExists(argc, argv, ADD_SURFACE_SCENE)) {
            addSurfaceScene(&myTree);
        }

        unsigned long nodeCount = myTree.getOctreeElementsCount();
        qDebug("Nodes after adding scenes: %ld nodes", nodeCount);

        myTree.writeToSVOFile("voxels.svo");
    }
    return 0;
}

void unitTest(VoxelTree * tree) {
    VoxelTreeElement* node = NULL;
    qDebug("unit tests...");
    unsigned long nodeCount;

    // We want our corner voxels to be about 1/2 meter high, and our TREE_SCALE is in meters, so...
    float voxelSize = 0.5f / TREE_SCALE;

    // Here's an example of how to create a voxel.
    qDebug("creating corner points...");
    tree->createVoxel(0, 0, 0, voxelSize, 255, 255 ,255);
    qDebug("Nodes at line %d... %ld nodes", __LINE__, tree->getOctreeElementsCount());


    // Here's an example of how to test if a voxel exists
    node = tree->getVoxelAt(0, 0, 0, voxelSize);
    if (node) {
        // and how to access it's color
        qDebug("CORRECT - corner point 0,0,0 exists... color is (%d,%d,%d) ",
            node->getColor()[0], node->getColor()[1], node->getColor()[2]);
    }

    qDebug("Nodes at line %d... %ld nodes", __LINE__, tree->getOctreeElementsCount());

    // here's an example of how to delete a voxel
    qDebug("attempting to delete corner point 0,0,0");
    tree->deleteVoxelAt(0, 0, 0, voxelSize);

    qDebug("Nodes at line %d... %ld nodes", __LINE__, tree->getOctreeElementsCount());

    // Test to see that the delete worked... it should be FALSE...
    if ((node = tree->getVoxelAt(0, 0, 0, voxelSize))) {
        qDebug("FAIL corner point 0,0,0 exists...");
    } else {
        qDebug("CORRECT corner point 0,0,0 does not exists...");
    }

    qDebug("Nodes at line %d... %ld nodes", __LINE__, tree->getOctreeElementsCount());

    tree->createVoxel(0, 0, 0, voxelSize, 255, 255 ,255);
    if ((node = tree->getVoxelAt(0, 0, 0, voxelSize))) {
        qDebug("CORRECT - corner point 0,0,0 exists... color is (%d,%d,%d) ",
            node->getColor()[0], node->getColor()[1], node->getColor()[2]);
    } else {
        qDebug("FAIL corner point 0,0,0 does not exists...");
    }

    qDebug("Nodes at line %d... %ld nodes", __LINE__, tree->getOctreeElementsCount());

    tree->createVoxel(voxelSize, 0, 0, voxelSize, 255, 255 ,0);
    if ((node = tree->getVoxelAt(voxelSize, 0, 0, voxelSize))) {
        qDebug("CORRECT - corner point voxelSize,0,0 exists... color is (%d,%d,%d) ",
            node->getColor()[0], node->getColor()[1], node->getColor()[2]);
    } else {
        qDebug("FAIL corner point voxelSize,0,0 does not exists...");
    }

    qDebug("Nodes at line %d... %ld nodes", __LINE__, tree->getOctreeElementsCount());

    tree->createVoxel(0, 0, voxelSize, voxelSize, 255, 0 ,0);
    if ((node = tree->getVoxelAt(0, 0, voxelSize, voxelSize))) {
        qDebug("CORRECT - corner point 0, 0, voxelSize exists... color is (%d,%d,%d) ",
            node->getColor()[0], node->getColor()[1], node->getColor()[2]);
    } else {
        qDebug("FAILED corner point 0, 0, voxelSize does not exists...");
    }

    qDebug("Nodes at line %d... %ld nodes", __LINE__, tree->getOctreeElementsCount());

    tree->createVoxel(voxelSize, 0, voxelSize, voxelSize, 0, 0 ,255);
    if ((node = tree->getVoxelAt(voxelSize, 0, voxelSize, voxelSize))) {
        qDebug("CORRECT - corner point voxelSize, 0, voxelSize exists... color is (%d,%d,%d) ",
            node->getColor()[0], node->getColor()[1], node->getColor()[2]);
    } else {
        qDebug("corner point voxelSize, 0, voxelSize does not exists...");
    }

    qDebug("Nodes at line %d... %ld nodes", __LINE__, tree->getOctreeElementsCount());

    qDebug("check root voxel exists...");
    if (tree->getVoxelAt(0,0,0,1.0)) {
        qDebug("of course it does");
    } else {
        qDebug("WTH!?!");
    }

    nodeCount = tree->getOctreeElementsCount();
    qDebug("Nodes before writing file: %ld nodes", nodeCount);

    tree->writeToSVOFile("voxels.svo");

    qDebug("erasing the tree...");
    tree->eraseAllOctreeElements();

    qDebug("check root voxel exists...");
    if (tree->getVoxelAt(0,0,0,1.0)) {
        qDebug("of course it does");
    } else {
        qDebug("WTH!?!");
    }

    // this should not exist... we just deleted it...
    if (tree->getVoxelAt(voxelSize, 0, voxelSize, voxelSize)) {
        qDebug("corner point voxelSize, 0, voxelSize exists...");
    } else {
        qDebug("corner point voxelSize, 0, voxelSize does not exists...");
    }

    tree->readFromSVOFile("voxels.svo");

    // this should exist... we just loaded it...
    if (tree->getVoxelAt(voxelSize, 0, voxelSize, voxelSize)) {
        qDebug("corner point voxelSize, 0, voxelSize exists...");
    } else {
        qDebug("corner point voxelSize, 0, voxelSize does not exists...");
    }

    nodeCount = tree->getOctreeElementsCount();
    qDebug("Nodes after loading file: %ld nodes", nodeCount);


}
