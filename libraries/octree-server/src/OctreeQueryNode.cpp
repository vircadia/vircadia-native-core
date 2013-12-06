//
//  OctreeQueryNode.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/21/13.
//
//

#include "PacketHeaders.h"
#include "SharedUtil.h"
#include "OctreeQueryNode.h"
#include <cstring>
#include <cstdio>
#include "OctreeSendThread.h"

OctreeQueryNode::OctreeQueryNode(Node* owningNode) :
    OctreeQuery(owningNode),
    _viewSent(false),
    _octreePacketAvailableBytes(MAX_PACKET_SIZE),
    _maxSearchLevel(1),
    _maxLevelReachedInLastSearch(1),
    _lastTimeBagEmpty(0),
    _viewFrustumChanging(false),
    _viewFrustumJustStoppedChanging(true),
    _currentPacketIsColor(true),
    _currentPacketIsCompressed(false),
    _octreeSendThread(NULL),
    _lastClientBoundaryLevelAdjust(0),
    _lastClientOctreeSizeScale(DEFAULT_OCTREE_SIZE_SCALE),
    _lodChanged(false),
    _lodInitialized(false)
{
    _octreePacket = new unsigned char[MAX_PACKET_SIZE];
    _octreePacketAt = _octreePacket;
    _lastOctreePacket = new unsigned char[MAX_PACKET_SIZE];
    _lastOctreePacketLength = 0;
    _duplicatePacketCount = 0;
    _sequenceNumber = 0;
}

void OctreeQueryNode::initializeOctreeSendThread(OctreeServer* octreeServer) {
    // Create octree sending thread...
    QUuid nodeUUID = getOwningNode()->getUUID();
    _octreeSendThread = new OctreeSendThread(nodeUUID, octreeServer);
    _octreeSendThread->initialize(true);
}

bool OctreeQueryNode::packetIsDuplicate() const {
    // since our packets now include header information, like sequence number, and createTime, we can't just do a memcmp
    // of the entire packet, we need to compare only the packet content...
    if (_lastOctreePacketLength == getPacketLength()) {
        return memcmp(_lastOctreePacket + OCTREE_PACKET_HEADER_SIZE,
                _octreePacket+OCTREE_PACKET_HEADER_SIZE , getPacketLength() - OCTREE_PACKET_HEADER_SIZE == 0);
    }
    return false;
}

bool OctreeQueryNode::shouldSuppressDuplicatePacket() {
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
            if (_duplicatePacketCount >= 1) {
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

void OctreeQueryNode::resetOctreePacket(bool lastWasSurpressed) {
    // Whenever we call this, we will keep a copy of the last packet, so we can determine if the last packet has
    // changed since we last reset it. Since we know that no two packets can ever be identical without being the same
    // scene information, (e.g. the root node packet of a static scene), we can use this as a strategy for reducing
    // packet send rate.
    _lastOctreePacketLength = getPacketLength();
    memcpy(_lastOctreePacket, _octreePacket, _lastOctreePacketLength);

    // If we're moving, and the client asked for low res, then we force monochrome, otherwise, use 
    // the clients requested color state.    
    _currentPacketIsColor = getWantColor();
    _currentPacketIsCompressed = getWantCompression();
    OCTREE_PACKET_FLAGS flags = 0;
    if (_currentPacketIsColor) {
        setAtBit(flags,PACKET_IS_COLOR_BIT);
    }
    if (_currentPacketIsCompressed) {
        setAtBit(flags,PACKET_IS_COMPRESSED_BIT);
    }

    _octreePacketAvailableBytes = MAX_PACKET_SIZE;
    int numBytesPacketHeader = populateTypeAndVersion(_octreePacket, getMyPacketType());
    _octreePacketAt = _octreePacket + numBytesPacketHeader;
    _octreePacketAvailableBytes -= numBytesPacketHeader;

    // pack in flags
    OCTREE_PACKET_FLAGS* flagsAt = (OCTREE_PACKET_FLAGS*)_octreePacketAt;
    *flagsAt = flags;
    _octreePacketAt += sizeof(OCTREE_PACKET_FLAGS);
    _octreePacketAvailableBytes -= sizeof(OCTREE_PACKET_FLAGS);

    // pack in sequence number
    OCTREE_PACKET_SEQUENCE* sequenceAt = (OCTREE_PACKET_SEQUENCE*)_octreePacketAt;
    *sequenceAt = _sequenceNumber;
    _octreePacketAt += sizeof(OCTREE_PACKET_SEQUENCE);
    _octreePacketAvailableBytes -= sizeof(OCTREE_PACKET_SEQUENCE);
    if (!(lastWasSurpressed || _lastOctreePacketLength == OCTREE_PACKET_HEADER_SIZE)) {
        _sequenceNumber++;
    }

    // pack in timestamp
    OCTREE_PACKET_SENT_TIME now = usecTimestampNow();
    OCTREE_PACKET_SENT_TIME* timeAt = (OCTREE_PACKET_SENT_TIME*)_octreePacketAt;
    *timeAt = now;
    _octreePacketAt += sizeof(OCTREE_PACKET_SENT_TIME);
    _octreePacketAvailableBytes -= sizeof(OCTREE_PACKET_SENT_TIME);

    _octreePacketWaiting = false;
}

void OctreeQueryNode::writeToPacket(const unsigned char* buffer, int bytes) {
    // compressed packets include lead bytes which contain compressed size, this allows packing of
    // multiple compressed portions together
    if (_currentPacketIsCompressed) {
        *(OCTREE_PACKET_INTERNAL_SECTION_SIZE*)_octreePacketAt = bytes;
        _octreePacketAt += sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);
        _octreePacketAvailableBytes -= sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);
    }
    if (bytes <= _octreePacketAvailableBytes) {
        memcpy(_octreePacketAt, buffer, bytes);
        _octreePacketAvailableBytes -= bytes;
        _octreePacketAt += bytes;
        _octreePacketWaiting = true;
    }    
}

OctreeQueryNode::~OctreeQueryNode() {
    delete[] _octreePacket;
    delete[] _lastOctreePacket;

    if (_octreeSendThread) {
        _octreeSendThread->terminate();
        delete _octreeSendThread;
    }
}

bool OctreeQueryNode::updateCurrentViewFrustum() {
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
        if (_lastClientOctreeSizeScale != getOctreeSizeScale()) {
            _lastClientOctreeSizeScale = getOctreeSizeScale();
            _lodChanged = true;
        }
    } else {
        _lodInitialized = true;
        _lastClientOctreeSizeScale = getOctreeSizeScale();
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

void OctreeQueryNode::setViewSent(bool viewSent) { 
    _viewSent = viewSent; 
    if (viewSent) {
        _viewFrustumJustStoppedChanging = false;
        _lodChanged = false;
    }
}

void OctreeQueryNode::updateLastKnownViewFrustum() {
    bool frustumChanges = !_lastKnownViewFrustum.isVerySimilar(_currentViewFrustum);
    
    if (frustumChanges) {
        // save our currentViewFrustum into our lastKnownViewFrustum
        _lastKnownViewFrustum = _currentViewFrustum;
    }
    
    // save that we know the view has been sent.
    uint64_t now = usecTimestampNow();
    setLastTimeBagEmpty(now); // is this what we want? poor names
}


bool OctreeQueryNode::moveShouldDump() const {
    glm::vec3 oldPosition = _lastKnownViewFrustum.getPosition();
    glm::vec3 newPosition = _currentViewFrustum.getPosition();

    // theoretically we could make this slightly larger but relative to avatar scale.    
    const float MAXIMUM_MOVE_WITHOUT_DUMP = 0.0f;
    if (glm::distance(newPosition, oldPosition) > MAXIMUM_MOVE_WITHOUT_DUMP) {
        return true;
    }
    return false;
}

void OctreeQueryNode::dumpOutOfView() {
    int stillInView = 0;
    int outOfView = 0;
    OctreeElementBag tempBag;
    while (!nodeBag.isEmpty()) {
        OctreeElement* node = nodeBag.extract();
        if (node->isInView(_currentViewFrustum)) {
            tempBag.insert(node);
            stillInView++;
        } else {
            outOfView++;
        }
    }
    if (stillInView > 0) {
        while (!tempBag.isEmpty()) {
            OctreeElement* node = tempBag.extract();
            if (node->isInView(_currentViewFrustum)) {
                nodeBag.insert(node);
            }
        }
    }
}

