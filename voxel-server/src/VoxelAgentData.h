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
#include "VoxelNodeBag.h"
#include "VoxelConstants.h"

class VoxelAgentData : public AvatarData {
public:
    VoxelAgentData();
    ~VoxelAgentData();
    VoxelAgentData(const VoxelAgentData &otherAgentData);
    
    VoxelAgentData* clone() const;

    void init(); // sets up data internals
    void resetVoxelPacket();  // resets voxel packet to after "V" header

    void writeToPacket(unsigned char* buffer, int bytes); // writes to end of packet

    const unsigned char* getPacket() const { return _voxelPacket; }
    int getPacketLength() const { return (MAX_VOXEL_PACKET_SIZE - _voxelPacketAvailableBytes); }
    bool isPacketWaiting() const { return _voxelPacketWaiting; }
    int getAvailable() const { return _voxelPacketAvailableBytes; }
    int getMaxSearchLevel() const { return _maxSearchLevel; };
    void resetMaxSearchLevel() { _maxSearchLevel = 1; };
    void incrementMaxSearchLevel() { _maxSearchLevel++; };

    int getMaxLevelReached() const { return _maxLevelReachedInLastSearch; };
    void setMaxLevelReached(int maxLevelReached) { _maxLevelReachedInLastSearch = maxLevelReached; }

    VoxelNodeBag nodeBag;

    ViewFrustum currentViewFrustum;
    ViewFrustum lastKnownViewFrustum;
    
    void updateViewFrustum();

private:    
    unsigned char* _voxelPacket;
    unsigned char* _voxelPacketAt;
    int _voxelPacketAvailableBytes;
    bool _voxelPacketWaiting;
    int _maxSearchLevel;
    int _maxLevelReachedInLastSearch;
};

#endif /* defined(__hifi__VoxelAgentData__) */
