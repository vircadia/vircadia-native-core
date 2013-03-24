//
//  VoxelTree.h
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelTree__
#define __hifi__VoxelTree__

#include "VoxelNode.h"

const int MAX_VOXEL_PACKET_SIZE = 1492;

class VoxelTree {
    VoxelNode * nodeForOctalCode(VoxelNode *ancestorNode, unsigned char * needleCode);
    VoxelNode * createMissingNode(VoxelNode *lastParentNode, unsigned char *deepestCodeToCreate);
    int readNodeData(VoxelNode *destinationNode, unsigned char * nodeData, int bufferSizeBytes);
public:
    VoxelTree();
    ~VoxelTree();
    
    VoxelNode *rootNode;
    
    void readBitstreamToTree(unsigned char * bitstream, int bufferSizeBytes);
    unsigned char * loadBitstreamBuffer(unsigned char *& bitstreamBuffer,
                               unsigned char * octalCode,
                               VoxelNode *currentVoxelNode);
    void printTreeForDebugging(VoxelNode *startNode);
    
};

#endif /* defined(__hifi__VoxelTree__) */
