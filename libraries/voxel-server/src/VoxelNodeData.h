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
    VoxelNodeData(Node* owningNode) : OctreeQueryNode(owningNode) {  };
    virtual PACKET_TYPE getMyPacketType() const { return PACKET_TYPE_VOXEL_DATA; }
};

#endif /* defined(__hifi__VoxelNodeData__) */
