//
//  VoxelNodeData.h
//  hifi
//
//  Created by Stephen Birarda on 3/21/13.
//
//

#ifndef __hifi__VoxelNodeData__
#define __hifi__VoxelNodeData__

#include <OctreeQueryNode.h>
#include <PacketHeaders.h>

class VoxelNodeData : public OctreeQueryNode {
public:
    VoxelNodeData() : OctreeQueryNode() {  };
    virtual PacketType getMyPacketType() const { return PacketTypeVoxelData; }
};

#endif /* defined(__hifi__VoxelNodeData__) */
