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
    _voxelPacket[0] = getWantColor() ? PACKET_HEADER_VOXEL_DATA : PACKET_HEADER_VOXEL_DATA_MONOCHROME;
    _voxelPacketAt = &_voxelPacket[1];
    _voxelPacketAvailableBytes = MAX_VOXEL_PACKET_SIZE - 1;
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

void VoxelAgentData::updateViewFrustum() {
    // save our currentViewFrustum into our lastKnownViewFrustum
    _lastKnownViewFrustum = _currentViewFrustum;

    // get position and orientation details from the camera
    _currentViewFrustum.setPosition(getCameraPosition());
    _currentViewFrustum.setOrientation(getCameraDirection(), getCameraUp(), getCameraRight());

    // Also make sure it's got the correct lens details from the camera
    _currentViewFrustum.setFieldOfView(getCameraFov());
    _currentViewFrustum.setAspectRatio(getCameraAspectRatio());
    _currentViewFrustum.setNearClip(getCameraNearClip());
    _currentViewFrustum.setFarClip(getCameraFarClip());
    
    // if there has been a change, then recalculate
    if (!_lastKnownViewFrustum.matches(_currentViewFrustum)) {
        _currentViewFrustum.calculate();
    }
}

