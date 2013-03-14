//
//  VoxelNode.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//
//

#include "VoxelNode.h"

VoxelNode::VoxelNode() {
    
    // default pointers to child nodes to NULL
    for (int i = 0; i < 8; i++) {
        children[i] = NULL;
    }
}



