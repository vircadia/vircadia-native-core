//
//  VoxelNodeData.h
//  hifi
//
//  Created by Stephen Birarda on 3/21/13.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelNodeData__
#define __hifi__VoxelNodeData__

#include <PacketHeaders.h>

#include "../octree/OctreeQueryNode.h"

class VoxelNodeData : public OctreeQueryNode {
public:
    VoxelNodeData() : OctreeQueryNode() {  };
    virtual PacketType getMyPacketType() const { return PacketTypeVoxelData; }
};

#endif /* defined(__hifi__VoxelNodeData__) */
