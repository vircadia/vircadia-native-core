//
//  VoxelTree.h
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelTree__
#define __hifi__VoxelTree__

#include <iostream>
#include "VoxelNode.h"

const int MAX_VOXEL_PACKET_SIZE = 1492;
const int MAX_TREE_SLICE_BYTES = 26;

class VoxelTree {
    VoxelNode * nodeForOctalCode(VoxelNode *ancestorNode, unsigned char * needleCode);
    VoxelNode * createMissingNode(VoxelNode *lastParentNode, unsigned char *deepestCodeToCreate);
    int readNodeData(VoxelNode *destinationNode, unsigned char * nodeData, int bufferSizeBytes);
public:
    VoxelTree();
    ~VoxelTree();
    
    VoxelNode *rootNode;
    int leavesWrittenToBitstream;
    
    int levelForViewerPosition(float * position);
    void readBitstreamToTree(unsigned char * bitstream, int bufferSizeBytes);
    void readCodeColorBufferToTree(unsigned char *codeColorBuffer);
    unsigned char * loadBitstreamBuffer(unsigned char *& bitstreamBuffer,
                                        unsigned char * octalCode,
                                        VoxelNode *currentVoxelNode,
                                        int deepestLevel);
    void printTreeForDebugging(VoxelNode *startNode);

	void pruneTree(VoxelNode* pruneAt);
	void loadVoxelsFile(const char* fileName, bool wantColorRandomizer);
	void createSphere(float r,float xc, float yc, float zc, float s, bool solid);
};

#endif /* defined(__hifi__VoxelTree__) */
