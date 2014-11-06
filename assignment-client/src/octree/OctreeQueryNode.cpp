//
//  OctreeQueryNode.cpp
//  assignment-client/src/octree
//
//  Created by Stephen Birarda on 3/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PacketHeaders.h"
#include "SharedUtil.h"
#include "OctreeQueryNode.h"
#include <cstring>
#include <cstdio>
#include "OctreeSendThread.h"

OctreeQueryNode::OctreeQueryNode() :
    _viewSent(false),
    _octreePacket(new unsigned char[MAX_PACKET_SIZE]),
    _octreePacketAt(_octreePacket),
    _octreePacketAvailableBytes(MAX_PACKET_SIZE),
    _octreePacketWaiting(false),
    _lastOctreePacket(new unsigned char[MAX_PACKET_SIZE]), 
    _lastOctreePacketLength(0),
    _duplicatePacketCount(0),
    _firstSuppressedPacket(usecTimestampNow()),
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
    _lodInitialized(false),
    _sequenceNumber(0),
    _lastRootTimestamp(0),
    _myPacketType(PacketTypeUnknown),
    _isShuttingDown(false),
    _sentPacketHistory()
{
}

OctreeQueryNode::~OctreeQueryNode() {
    _isShuttingDown = true;
    if (_octreeSendThread) {
        forceNodeShutdown();
    }
    
    delete[] _octreePacket;
    delete[] _lastOctreePacket;
}

void OctreeQueryNode::nodeKilled() {
    _isShuttingDown = true;
    elementBag.unhookNotifications(); // if our node is shutting down, then we no longer need octree element notifications
    if (_octreeSendThread) {
        // just tell our thread we want to shutdown, this is asynchronous, and fast, we don't need or want it to block
        // while the thread actually shuts down
        _octreeSendThread->setIsShuttingDown();
    }
}

void OctreeQueryNode::forceNodeShutdown() {
    _isShuttingDown = true;
    elementBag.unhookNotifications(); // if our node is shutting down, then we no longer need octree element notifications
    if (_octreeSendThread) {
        // we really need to force our thread to shutdown, this is synchronous, we will block while the thread actually 
        // shuts down because we really need it to shutdown, and it's ok if we wait for it to complete
        OctreeSendThread* sendThread = _octreeSendThread;
        _octreeSendThread = NULL;
        sendThread->setIsShuttingDown();
        sendThread->terminate();
        delete sendThread;
    }
}

void OctreeQueryNode::sendThreadFinished() {
    // We've been notified by our thread that it is shutting down. So we can clean up our reference to it, and
    // delete the actual thread object. Cleaning up our thread will correctly unroll all refereces to shared
    // pointers to our node as well as the octree server assignment
    if (_octreeSendThread) {
        OctreeSendThread* sendThread = _octreeSendThread;
        _octreeSendThread = NULL;
        delete sendThread;
    }
}

void OctreeQueryNode::initializeOctreeSendThread(const SharedAssignmentPointer& myAssignment, const SharedNodePointer& node) {
    _octreeSendThread = new OctreeSendThread(myAssignment, node);
    
    // we want to be notified when the thread finishes
    connect(_octreeSendThread, &GenericThread::finished, this, &OctreeQueryNode::sendThreadFinished);
    _octreeSendThread->initialize(true);
}

bool OctreeQueryNode::packetIsDuplicate() const {
    // if shutting down, return immediately
    if (_isShuttingDown) {
        return false;
    }
    // since our packets now include header information, like sequence number, and createTime, we can't just do a memcmp
    // of the entire packet, we need to compare only the packet content...
    int numBytesPacketHeader = numBytesForPacketHeaderGivenPacketType(_myPacketType);
    
    if (_lastOctreePacketLength == getPacketLength()) {
        if (memcmp(_lastOctreePacket + (numBytesPacketHeader + OCTREE_PACKET_EXTRA_HEADERS_SIZE),
                _octreePacket + (numBytesPacketHeader + OCTREE_PACKET_EXTRA_HEADERS_SIZE),
                   getPacketLength() - (numBytesPacketHeader + OCTREE_PACKET_EXTRA_HEADERS_SIZE)) == 0) {
            return true;
        }
    }
    return false;
}

bool OctreeQueryNode::shouldSuppressDuplicatePacket() {
    // if shutting down, return immediately
    if (_isShuttingDown) {
        return true;
    }

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
        quint64 now = usecTimestampNow();
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

void OctreeQueryNode::init() {
    _myPacketType = getMyPacketType();
    resetOctreePacket(); // don't bump sequence
}


void OctreeQueryNode::resetOctreePacket() {
    // if shutting down, return immediately
    if (_isShuttingDown) {
        return;
    }

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
    int numBytesPacketHeader = populatePacketHeader(reinterpret_cast<char*>(_octreePacket), _myPacketType);
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

    // pack in timestamp
    OCTREE_PACKET_SENT_TIME now = usecTimestampNow();
    OCTREE_PACKET_SENT_TIME* timeAt = (OCTREE_PACKET_SENT_TIME*)_octreePacketAt;
    *timeAt = now;
    _octreePacketAt += sizeof(OCTREE_PACKET_SENT_TIME);
    _octreePacketAvailableBytes -= sizeof(OCTREE_PACKET_SENT_TIME);

    _octreePacketWaiting = false;
}

void OctreeQueryNode::writeToPacket(const unsigned char* buffer, unsigned int bytes) {
    // if shutting down, return immediately
    if (_isShuttingDown) {
        return;
    }

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

bool OctreeQueryNode::updateCurrentViewFrustum() {
    // if shutting down, return immediately
    if (_isShuttingDown) {
        return false;
    }

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
    // if shutting down, return immediately
    if (_isShuttingDown) {
        return;
    }

    bool frustumChanges = !_lastKnownViewFrustum.isVerySimilar(_currentViewFrustum);

    if (frustumChanges) {
        // save our currentViewFrustum into our lastKnownViewFrustum
        _lastKnownViewFrustum = _currentViewFrustum;
    }

    // save that we know the view has been sent.
    quint64 now = usecTimestampNow();
    setLastTimeBagEmpty(now); // is this what we want? poor names
    //qDebug() << "{" <<  QThread::currentThread() << "} " << "        nodeData->setLastTimeBagEmpty() now:" << now;
}


bool OctreeQueryNode::moveShouldDump() const {
    // if shutting down, return immediately
    if (_isShuttingDown) {
        return false;
    }

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
    // if shutting down, return immediately
    if (_isShuttingDown) {
        return;
    }
    
    int stillInView = 0;
    int outOfView = 0;
    OctreeElementBag tempBag;
    while (!elementBag.isEmpty()) {
        OctreeElement* node = elementBag.extract();
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
                elementBag.insert(node);
            }
        }
    }
}

void OctreeQueryNode::octreePacketSent() {
    packetSent(_octreePacket, getPacketLength());
}

void OctreeQueryNode::packetSent(unsigned char* packet, int packetLength) {
    packetSent(QByteArray((char*)packet, packetLength));
}

void OctreeQueryNode::packetSent(const QByteArray& packet) {
    _sentPacketHistory.packetSent(_sequenceNumber, packet);
    _sequenceNumber++;
}

bool OctreeQueryNode::hasNextNackedPacket() const {
    return !_nackedSequenceNumbers.isEmpty();
}

const QByteArray* OctreeQueryNode::getNextNackedPacket() {
    if (!_nackedSequenceNumbers.isEmpty()) {
        // could return null if packet is not in the history
        return _sentPacketHistory.getPacket(_nackedSequenceNumbers.dequeue());
    }
    return NULL;
}

void OctreeQueryNode::parseNackPacket(const QByteArray& packet) {

    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    const unsigned char* dataAt = reinterpret_cast<const unsigned char*>(packet.data()) + numBytesPacketHeader;

    // read number of sequence numbers
    uint16_t numSequenceNumbers = (*(uint16_t*)dataAt);
    dataAt += sizeof(uint16_t);

    // read sequence numbers
    for (int i = 0; i < numSequenceNumbers; i++) {
        OCTREE_PACKET_SEQUENCE sequenceNumber = (*(OCTREE_PACKET_SEQUENCE*)dataAt);
        _nackedSequenceNumbers.enqueue(sequenceNumber);
        dataAt += sizeof(OCTREE_PACKET_SEQUENCE);
    }
}
