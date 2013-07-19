//
//  VoxelNodeData.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/21/13.
//
//

#include "PacketHeaders.h"
#include "SharedUtil.h"
#include "VoxelNodeData.h"
#include <cstring>
#include <cstdio>

VoxelNodeData::VoxelNodeData(Node* owningNode) :
    AvatarData(owningNode),
    _viewSent(false),
    _voxelPacketAvailableBytes(MAX_VOXEL_PACKET_SIZE),
    _maxSearchLevel(1),
    _maxLevelReachedInLastSearch(1),
    _lastTimeBagEmpty(0),
    _viewFrustumChanging(false),
    _viewFrustumJustStoppedChanging(true),
    _currentPacketIsColor(true)
{
    _voxelPacket = new unsigned char[MAX_VOXEL_PACKET_SIZE];
    _voxelPacketAt = _voxelPacket;
    resetVoxelPacket();
}


void VoxelNodeData::resetVoxelPacket() {
    // If we're moving, and the client asked for low res, then we force monochrome, otherwise, use 
    // the clients requested color state.    
    _currentPacketIsColor = (LOW_RES_MONO && getWantLowResMoving() && _viewFrustumChanging) ? false : getWantColor();
    PACKET_TYPE voxelPacketType = _currentPacketIsColor ? PACKET_TYPE_VOXEL_DATA : PACKET_TYPE_VOXEL_DATA_MONOCHROME;
    int numBytesPacketHeader = populateTypeAndVersion(_voxelPacket, voxelPacketType);
    _voxelPacketAt = _voxelPacket + numBytesPacketHeader;
    _voxelPacketAvailableBytes = MAX_VOXEL_PACKET_SIZE - numBytesPacketHeader;
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
    
    // When we first detect that the view stopped changing, we record this.
    // but we don't change it back to false until we've completely sent this
    // scene.
    if (_viewFrustumChanging && !currentViewFrustumChanged) {
        _viewFrustumJustStoppedChanging = true;
    }
    _viewFrustumChanging = currentViewFrustumChanged;
    return currentViewFrustumChanged;
}

void VoxelNodeData::setViewSent(bool viewSent) { 
    _viewSent = viewSent; 
    if (viewSent) {
        _viewFrustumJustStoppedChanging = false;
    }
}


void VoxelNodeData::updateLastKnownViewFrustum() {
    bool frustumChanges = !_lastKnownViewFrustum.matches(_currentViewFrustum);
    
    if (frustumChanges) {
        // save our currentViewFrustum into our lastKnownViewFrustum
        _lastKnownViewFrustum = _currentViewFrustum;
    }
    
    // save that we know the view has been sent.
    uint64_t now = usecTimestampNow();
    setLastTimeBagEmpty(now); // is this what we want? poor names
}

