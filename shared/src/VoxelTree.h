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

class VoxelTree {
public:
    VoxelTree();
    
    VoxelNode *rootNode;
    
    unsigned char * loadBitstreamBuffer(char *& bitstreamBuffer,
                               unsigned char * octalCode,
                               VoxelNode *currentVoxelNode);
    
};

#endif /* defined(__hifi__VoxelTree__) */
