//
//  VoxelAgentData.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/21/13.
//
//

#include "PacketHeaders.h"
#include "VoxelAgentData.h"
#include <cstring>
#include <cstdio>

VoxelAgentData::VoxelAgentData() {
    init();
}

void VoxelAgentData::init() {
    _voxelPacket = new unsigned char[MAX_VOXEL_PACKET_SIZE];
    _voxelPacketAvailableBytes = MAX_VOXEL_PACKET_SIZE;
    _voxelPacketAt = _voxelPacket;
    _maxSearchLevel = 1;
    _maxLevelReachedInLastSearch = 1;
    resetVoxelPacket();
}

void VoxelAgentData::resetVoxelPacket() {
    _voxelPacket[0] = PACKET_HEADER_VOXEL_DATA;
    _voxelPacketAt = &_voxelPacket[1];
    _voxelPacketAvailableBytes = MAX_VOXEL_PACKET_SIZE-1;
    _voxelPacketWaiting = false;
}

void VoxelAgentData::writeToPacket(unsigned char* buffer, int bytes) {
    memcpy(_voxelPacketAt, buffer, bytes);
    _voxelPacketAvailableBytes -= bytes;
    _voxelPacketAt += bytes;
    _voxelPacketWaiting = true;
}

VoxelAgentData::~VoxelAgentData() {
    delete[] _voxelPacket;
}

VoxelAgentData::VoxelAgentData(const VoxelAgentData &otherAgentData) {
    memcpy(&_position, &otherAgentData._position, sizeof(_position));
    init();
}

VoxelAgentData* VoxelAgentData::clone() const {
    return new VoxelAgentData(*this);
}
