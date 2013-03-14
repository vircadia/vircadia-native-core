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

class VoxelTree {
public:
    VoxelTree();
    
    VoxelNode *rootNode;
    
    
    void addBitstreamPacketMarker();
    
    void outputDebugInformation(VoxelNode *currentRootNode = NULL);    
    unsigned char * loadBitstream(unsigned char * bitstreamBuffer, VoxelNode *bitstreamRootNode, unsigned char ** curPacketSplitPtr = NULL);
};

#endif /* defined(__hifi__VoxelTree__) */
