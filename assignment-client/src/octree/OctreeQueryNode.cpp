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

#include "OctreeQueryNode.h"

#include <cstring>
#include <cstdio>

#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "OctreeSendThread.h"

void OctreeQueryNode::nodeKilled() {
    _isShuttingDown = true;
}

bool OctreeQueryNode::packetIsDuplicate() const {
    // if shutting down, return immediately
    if (_isShuttingDown) {
        return false;
    }
    // since our packets now include header information, like sequence number, and createTime, we can't just do a memcmp
    // of the entire packet, we need to compare only the packet content...

    if (_lastOctreePacketLength == _octreePacket->getPayloadSize()) {
        if (memcmp(_lastOctreePayload.data() + OCTREE_PACKET_EXTRA_HEADERS_SIZE,
                   _octreePacket->getPayload() + OCTREE_PACKET_EXTRA_HEADERS_SIZE,
                   _octreePacket->getPayloadSize() - OCTREE_PACKET_EXTRA_HEADERS_SIZE) == 0) {
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

    _octreePacket = NLPacket::create(getMyPacketType());

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
    _lastOctreePacketLength = _octreePacket->getPayloadSize();
    memcpy(_lastOctreePayload.data(), _octreePacket->getPayload(), _lastOctreePacketLength);

    // If we're moving, and the client asked for low res, then we force monochrome, otherwise, use
    // the clients requested color state.
    OCTREE_PACKET_FLAGS flags = 0;
    setAtBit(flags, PACKET_IS_COLOR_BIT); // always color
    setAtBit(flags, PACKET_IS_COMPRESSED_BIT); // always compressed

    _octreePacket->reset();

    // pack in flags
    _octreePacket->writePrimitive(flags);

    // pack in sequence number
    _octreePacket->writePrimitive(_sequenceNumber);

    // pack in timestamp
    OCTREE_PACKET_SENT_TIME now = usecTimestampNow();
    _octreePacket->writePrimitive(now);

    _octreePacketWaiting = false;
}

void OctreeQueryNode::writeToPacket(const unsigned char* buffer, unsigned int bytes) {
    // if shutting down, return immediately
    if (_isShuttingDown) {
        return;
    }

    // compressed packets include lead bytes which contain compressed size, this allows packing of
    // multiple compressed portions together
    OCTREE_PACKET_INTERNAL_SECTION_SIZE sectionSize = bytes;
    _octreePacket->writePrimitive(sectionSize);

    if (bytes <= _octreePacket->bytesAvailableForWrite()) {
        _octreePacket->write(reinterpret_cast<const char*>(buffer), bytes);
        _octreePacketWaiting = true;
    }
}

void OctreeQueryNode::copyCurrentViewFrustum(ViewFrustum& viewOut) const {
    QMutexLocker viewLocker(&_viewMutex);
    viewOut = _currentViewFrustum;
}

void OctreeQueryNode::copyLastKnownViewFrustum(ViewFrustum& viewOut) const {
    QMutexLocker viewLocker(&_viewMutex);
    viewOut = _lastKnownViewFrustum;
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

    newestViewFrustum.setCenterRadius(getCameraCenterRadius());

    // Also make sure it's got the correct lens details from the camera
    float originalFOV = getCameraFov();
    float wideFOV = originalFOV + VIEW_FRUSTUM_FOV_OVERSEND;

    if (0.0f != getCameraAspectRatio() &&
        0.0f != getCameraNearClip() &&
        0.0f != getCameraFarClip() &&
        getCameraNearClip() != getCameraFarClip()) {
        newestViewFrustum.setProjection(glm::perspective(
            glm::radians(wideFOV), // hack
            getCameraAspectRatio(),
            getCameraNearClip(),
            getCameraFarClip()));
    }


    { // if there has been a change, then recalculate
        QMutexLocker viewLocker(&_viewMutex);
        if (!newestViewFrustum.isVerySimilar(_currentViewFrustum)) {
            _currentViewFrustum = newestViewFrustum;
            _currentViewFrustum.calculate();
            currentViewFrustumChanged = true;
        }
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

    {
        QMutexLocker viewLocker(&_viewMutex);
        bool frustumChanges = !_lastKnownViewFrustum.isVerySimilar(_currentViewFrustum);

        if (frustumChanges) {
            // save our currentViewFrustum into our lastKnownViewFrustum
            _lastKnownViewFrustum = _currentViewFrustum;
        }
    }

    // save that we know the view has been sent.
    setLastTimeBagEmpty();
}


bool OctreeQueryNode::moveShouldDump() const {
    // if shutting down, return immediately
    if (_isShuttingDown) {
        return false;
    }

    QMutexLocker viewLocker(&_viewMutex);
    glm::vec3 oldPosition = _lastKnownViewFrustum.getPosition();
    glm::vec3 newPosition = _currentViewFrustum.getPosition();

    // theoretically we could make this slightly larger but relative to avatar scale.
    const float MAXIMUM_MOVE_WITHOUT_DUMP = 0.0f;
    return glm::distance(newPosition, oldPosition) > MAXIMUM_MOVE_WITHOUT_DUMP;
}

void OctreeQueryNode::dumpOutOfView() {
    // if shutting down, return immediately
    if (_isShuttingDown) {
        return;
    }

    int stillInView = 0;
    int outOfView = 0;
    OctreeElementBag tempBag;
    ViewFrustum viewCopy;
    copyCurrentViewFrustum(viewCopy);
    while (OctreeElementPointer elementToCheck = elementBag.extract()) {
        if (elementToCheck->isInView(viewCopy)) {
            tempBag.insert(elementToCheck);
            stillInView++;
        } else {
            outOfView++;
        }
    }
    if (stillInView > 0) {
        while (OctreeElementPointer elementToKeepInBag = tempBag.extract()) {
            if (elementToKeepInBag->isInView(viewCopy)) {
                elementBag.insert(elementToKeepInBag);
            }
        }
    }
}

void OctreeQueryNode::packetSent(const NLPacket& packet) {
    _sentPacketHistory.packetSent(_sequenceNumber, packet);
    _sequenceNumber++;
}

bool OctreeQueryNode::hasNextNackedPacket() const {
    return !_nackedSequenceNumbers.isEmpty();
}

const NLPacket* OctreeQueryNode::getNextNackedPacket() {
    if (!_nackedSequenceNumbers.isEmpty()) {
        // could return null if packet is not in the history
        return _sentPacketHistory.getPacket(_nackedSequenceNumbers.dequeue());
    }

    return nullptr;
}

void OctreeQueryNode::parseNackPacket(ReceivedMessage& message) {
    // read sequence numbers
    while (message.getBytesLeftToRead()) {
        OCTREE_PACKET_SEQUENCE sequenceNumber;
        message.readPrimitive(&sequenceNumber);
        _nackedSequenceNumbers.enqueue(sequenceNumber);
    }
}
