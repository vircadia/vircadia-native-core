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
#include "VoxelSendThread.h"

VoxelNodeData::VoxelNodeData(Node* owningNode) :
    VoxelQuery(owningNode),
    _viewSent(false),
    _voxelPacketAvailableBytes(MAX_VOXEL_PACKET_SIZE),
    _maxSearchLevel(1),
    _maxLevelReachedInLastSearch(1),
    _lastTimeBagEmpty(0),
    _viewFrustumChanging(false),
    _viewFrustumJustStoppedChanging(true),
    _currentPacketIsColor(true),
    _voxelSendThread(NULL),
    _lastClientBoundaryLevelAdjust(0),
    _lastClientVoxelSizeScale(DEFAULT_VOXEL_SIZE_SCALE),
    _lodChanged(false),
    _lodInitialized(false)
{
    _voxelPacket = new unsigned char[MAX_VOXEL_PACKET_SIZE];
    _voxelPacketAt = _voxelPacket;
    _lastVoxelPacket = new unsigned char[MAX_VOXEL_PACKET_SIZE];
    _lastVoxelPacketLength = 0;
    _duplicatePacketCount = 0;
    resetVoxelPacket();
    
    qDebug("VoxelNodeData::VoxelNodeData() this=%p owningNode=%p\n", this, owningNode);
}

void VoxelNodeData::initializeVoxelSendThread(VoxelServer* voxelServer) {
    // Create voxel sending thread...
    QUuid nodeUUID = getOwningNode()->getUUID();
    _voxelSendThread = new VoxelSendThread(nodeUUID, voxelServer);
    _voxelSendThread->initialize(true);
    
    qDebug("VoxelNodeData::initializeVoxelSendThread() this=%p owningNode=%p _voxelSendThread=%p\n", 
        this, getOwningNode(), _voxelSendThread);
    qDebug() << "VoxelNodeData::initializeVoxelSendThread() nodeUUID=" << nodeUUID << "\n";
}

bool VoxelNodeData::packetIsDuplicate() const {
    if (_lastVoxelPacketLength == getPacketLength()) {
        return memcmp(_lastVoxelPacket, _voxelPacket, getPacketLength()) == 0;
    }
    return false;
}

bool VoxelNodeData::shouldSuppressDuplicatePacket() {
    bool shouldSuppress = false; // assume we won't suppress
    
    // only consider duplicate packets
    if (packetIsDuplicate()) {
        _duplicatePacketCount++;

        // If this is the first suppressed packet, remember our time...    
        if (_duplicatePacketCount == 1) {
            _firstSuppressedPacket = usecTimestampNow();
        }

        // How long has it been since we've sent one, if we're still under our max time, then keep considering
        // this packet for suppression
        uint64_t now = usecTimestampNow();
        long sinceFirstSuppressedPacket = now - _firstSuppressedPacket;
        const long MAX_TIME_BETWEEN_DUPLICATE_PACKETS = 1000 * 1000; // 1 second.

        if (sinceFirstSuppressedPacket < MAX_TIME_BETWEEN_DUPLICATE_PACKETS) {
            // Finally, if we know we've sent at least one duplicate out, then suppress the rest...
            if (_duplicatePacketCount > 1) {
                shouldSuppress = true;
            }
        } else {
            // Reset our count, we've reached our maximum time.
            _duplicatePacketCount = 0;
        }
    } else {
        // Reset our count, it wasn't a duplicate
        _duplicatePacketCount = 0;
    }
    return shouldSuppress;
}

void VoxelNodeData::resetVoxelPacket() {
    // Whenever we call this, we will keep a copy of the last packet, so we can determine if the last packet has
    // changed since we last reset it. Since we know that no two packets can ever be identical without being the same
    // scene information, (e.g. the root node packet of a static scene), we can use this as a strategy for reducing
    // packet send rate.
    _lastVoxelPacketLength = getPacketLength();
    memcpy(_lastVoxelPacket, _voxelPacket, _lastVoxelPacketLength);

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

    qDebug("VoxelNodeData::~VoxelNodeData() this=%p owningNode=%p _voxelSendThread=%p\n", 
        this, getOwningNode(), _voxelSendThread);
    QUuid nodeUUID = getOwningNode()->getUUID();
    qDebug() << "VoxelNodeData::~VoxelNodeData() nodeUUID=" << nodeUUID << "\n";

    delete[] _voxelPacket;
    delete[] _lastVoxelPacket;

    if (_voxelSendThread) {
        _voxelSendThread->terminate();
        delete _voxelSendThread;
    }
}

bool VoxelNodeData::updateCurrentViewFrustum() {
    bool currentViewFrustumChanged = false;
    ViewFrustum newestViewFrustum;
    // get position and orientation details from the camera
    newestViewFrustum.setPosition(getCameraPosition());
    newestViewFrustum.setOrientation(getCameraOrientation());

    // Also make sure it's got the correct lens details from the camera
    float originalFOV = getCameraFov();
    float wideFOV = originalFOV + VIEW_FRUSTUM_FOV_OVERSEND;
    
    newestViewFrustum.setFieldOfView(wideFOV); // hack
    newestViewFrustum.setAspectRatio(getCameraAspectRatio());
    newestViewFrustum.setNearClip(getCameraNearClip());
    newestViewFrustum.setFarClip(getCameraFarClip());
    newestViewFrustum.setEyeOffsetPosition(getCameraEyeOffsetPosition());
    
    // if there has been a change, then recalculate
    if (!newestViewFrustum.isVerySimilar(_currentViewFrustum)) {
        _currentViewFrustum = newestViewFrustum;
        _currentViewFrustum.calculate();
        currentViewFrustumChanged = true;
    }
    
    // Also check for LOD changes from the client
    if (_lodInitialized) {
        if (_lastClientBoundaryLevelAdjust != getBoundaryLevelAdjust()) {
            _lastClientBoundaryLevelAdjust = getBoundaryLevelAdjust();
            _lodChanged = true;
        }
        if (_lastClientVoxelSizeScale != getVoxelSizeScale()) {
            _lastClientVoxelSizeScale = getVoxelSizeScale();
            _lodChanged = true;
        }
    } else {
        _lodInitialized = true;
        _lastClientVoxelSizeScale = getVoxelSizeScale();
        _lastClientBoundaryLevelAdjust = getBoundaryLevelAdjust();
        _lodChanged = false;
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
        _lodChanged = false;
    }
}


void VoxelNodeData::updateLastKnownViewFrustum() {
    bool frustumChanges = !_lastKnownViewFrustum.isVerySimilar(_currentViewFrustum);
    
    if (frustumChanges) {
        // save our currentViewFrustum into our lastKnownViewFrustum
        _lastKnownViewFrustum = _currentViewFrustum;
    }
    
    // save that we know the view has been sent.
    uint64_t now = usecTimestampNow();
    setLastTimeBagEmpty(now); // is this what we want? poor names
}


bool VoxelNodeData::moveShouldDump() const {
    glm::vec3 oldPosition = _lastKnownViewFrustum.getPosition();
    glm::vec3 newPosition = _currentViewFrustum.getPosition();

    // theoretically we could make this slightly larger but relative to avatar scale.    
    const float MAXIMUM_MOVE_WITHOUT_DUMP = 0.0f;
    if (glm::distance(newPosition, oldPosition) > MAXIMUM_MOVE_WITHOUT_DUMP) {
        return true;
    }
    return false;
}

void VoxelNodeData::dumpOutOfView() {
    int stillInView = 0;
    int outOfView = 0;
    VoxelNodeBag tempBag;
    while (!nodeBag.isEmpty()) {
        VoxelNode* node = nodeBag.extract();
        if (node->isInView(_currentViewFrustum)) {
            tempBag.insert(node);
            stillInView++;
        } else {
            outOfView++;
        }
    }
    if (stillInView > 0) {
        while (!tempBag.isEmpty()) {
            VoxelNode* node = tempBag.extract();
            if (node->isInView(_currentViewFrustum)) {
                nodeBag.insert(node);
            }
        }
    }
}

