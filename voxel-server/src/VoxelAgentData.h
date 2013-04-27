//
//  VoxelAgentData.h
//  hifi
//
//  Created by Stephen Birarda on 3/21/13.
//
//

#ifndef __hifi__VoxelAgentData__
#define __hifi__VoxelAgentData__

#include <iostream>
#include <AgentData.h>
#include <AvatarData.h>
#include "MarkerNode.h"
#include "VoxelNodeBag.h"
#include "VoxelTree.h" // for MAX_VOXEL_PACKET_SIZE

class VoxelAgentData : public AvatarData {
public:
    MarkerNode* rootMarkerNode;

    VoxelAgentData();
    ~VoxelAgentData();
    VoxelAgentData(const VoxelAgentData &otherAgentData);
    
    VoxelAgentData* clone() const;

    void init(); // sets up data internals
    void resetVoxelPacket();  // resets voxel packet to after "V" header

    void writeToPacket(unsigned char* buffer, int bytes); // writes to end of packet

    const unsigned char* getPacket() const { return _voxelPacket; }
    int getPacketLength() const { return (MAX_VOXEL_PACKET_SIZE-_voxelPacketAvailableBytes); }
    bool isPacketWaiting() const { return _voxelPacketWaiting; }
    int getAvailable() const { return _voxelPacketAvailableBytes; }

    VoxelNodeBag nodeBag;
private:    
    unsigned char* _voxelPacket;
    unsigned char* _voxelPacketAt;
    int _voxelPacketAvailableBytes;
    bool _voxelPacketWaiting;
    
    
};

#endif /* defined(__hifi__VoxelAgentData__) */
