//
//  VoxelNodeData.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/21/13.
//
//

#include "PacketHeaders.h"
#include "VoxelNodeData.h"
#include <cstring>
#include <cstdio>

VoxelNodeData::VoxelNodeData(Node* owningNode) :
    AvatarData(owningNode),
    _viewSent(false),
    _voxelPacketAvailableBytes(MAX_VOXEL_PACKET_SIZE),
    _maxSearchLevel(1),
    _maxLevelReachedInLastSearch(1),
    _lastTimeBagEmpty(0)
{
    _voxelPacket = new unsigned char[MAX_VOXEL_PACKET_SIZE];
    _voxelPacketAt = _voxelPacket;
    
    resetVoxelPacket();
}


void VoxelNodeData::resetVoxelPacket() {
    _voxelPacket[0] = getWantColor() ? PACKET_HEADER_VOXEL_DATA : PACKET_HEADER_VOXEL_DATA_MONOCHROME;
    _voxelPacketAt = &_voxelPacket[1];
    _voxelPacketAvailableBytes = MAX_VOXEL_PACKET_SIZE - 1;
    _voxelPacketWaiting = false;
}

void VoxelNodeData::writeToPacket(unsigned char* buffer, int bytes) {
    memcpy(_voxelPacketAt, buffer, bytes);
    _voxelPacketAvailableBytes -= bytes;
    _voxelPacketAt += bytes;
    _voxelPacketWaiting = true;
}

VoxelNodeData::~VoxelNodeData() {
    delete[] _voxelPacket;
}

bool VoxelNodeData::updateCurrentViewFrustum() {
    bool currentViewFrustumChanged = false;
    ViewFrustum newestViewFrustum;
    // get position and orientation details from the camera
    newestViewFrustum.setPosition(getCameraPosition());
    newestViewFrustum.setOrientation(getCameraOrientation());

    // Also make sure it's got the correct lens details from the camera
    newestViewFrustum.setFieldOfView(getCameraFov());
    newestViewFrustum.setAspectRatio(getCameraAspectRatio());
    newestViewFrustum.setNearClip(getCameraNearClip());
    newestViewFrustum.setFarClip(getCameraFarClip());
    
    // if there has been a change, then recalculate
    if (!newestViewFrustum.matches(_currentViewFrustum)) {
        _currentViewFrustum = newestViewFrustum;
        _currentViewFrustum.calculate();
        currentViewFrustumChanged = true;
    }
    return currentViewFrustumChanged;
}

void VoxelNodeData::updateLastKnownViewFrustum() {
    bool frustumChanges = !_lastKnownViewFrustum.matches(_currentViewFrustum);
    
    if (frustumChanges) {
        // save our currentViewFrustum into our lastKnownViewFrustum
        _lastKnownViewFrustum = _currentViewFrustum;
    }
}

