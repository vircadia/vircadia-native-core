//
//  VoxelNodeData.h
//  assignment-client/src/voxels
//
//  Created by Stephen Birarda on 3/21/13.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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
