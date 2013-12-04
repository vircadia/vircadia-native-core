//
//  ParticleReceiverData.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/2/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "PacketHeaders.h"
#include "SharedUtil.h"
#include "ParticleReceiverData.h"
#include <cstring>
#include <cstdio>
#include "ParticleSendThread.h"

ParticleReceiverData::ParticleReceiverData(Node* owningNode) :
    ParticleQuery(owningNode),
    _viewSent(false),
    _particlePacketAvailableBytes(MAX_PACKET_SIZE),
    _maxSearchLevel(1),
    _maxLevelReachedInLastSearch(1),
    _lastTimeBagEmpty(0),
    _viewFrustumChanging(false),
    _viewFrustumJustStoppedChanging(true),
    _currentPacketIsColor(true),
    _currentPacketIsCompressed(false),
    _particleSendThread(NULL),
    _lastClientBoundaryLevelAdjust(0),
    _lastClientParticleSizeScale(DEFAULT_PARTICLE_SIZE_SCALE),
    _lodChanged(false),
    _lodInitialized(false)
{
    _particlePacket = new unsigned char[MAX_PACKET_SIZE];
    _particlePacketAt = _particlePacket;
    _lastParticlePacket = new unsigned char[MAX_PACKET_SIZE];
    _lastParticlePacketLength = 0;
    _duplicatePacketCount = 0;
    _sequenceNumber = 0;
    resetParticlePacket(true); // don't bump sequence
}

void ParticleReceiverData::initializeParticleSendThread(ParticleServer* particleServer) {
    // Create particle sending thread...
    QUuid nodeUUID = getOwningNode()->getUUID();
    _particleSendThread = new ParticleSendThread(nodeUUID, particleServer);
    _particleSendThread->initialize(true);
}

bool ParticleReceiverData::packetIsDuplicate() const {
    // since our packets now include header information, like sequence number, and createTime, we can't just do a memcmp
    // of the entire packet, we need to compare only the packet content...
    if (_lastParticlePacketLength == getPacketLength()) {
        return memcmp(_lastParticlePacket + PARTICLE_PACKET_HEADER_SIZE, 
                _particlePacket+PARTICLE_PACKET_HEADER_SIZE , getPacketLength() - PARTICLE_PACKET_HEADER_SIZE) == 0;
    }
    return false;
}

bool ParticleReceiverData::shouldSuppressDuplicatePacket() {
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

void ParticleReceiverData::resetParticlePacket(bool lastWasSurpressed) {
    // Whenever we call this, we will keep a copy of the last packet, so we can determine if the last packet has
    // changed since we last reset it. Since we know that no two packets can ever be identical without being the same
    // scene information, (e.g. the root node packet of a static scene), we can use this as a strategy for reducing
    // packet send rate.
    _lastParticlePacketLength = getPacketLength();
    memcpy(_lastParticlePacket, _particlePacket, _lastParticlePacketLength);

    // If we're moving, and the client asked for low res, then we force monochrome, otherwise, use 
    // the clients requested color state.    
    _currentPacketIsColor = getWantColor();
    _currentPacketIsCompressed = getWantCompression();
    PARTICLE_PACKET_FLAGS flags = 0;
    if (_currentPacketIsColor) {
        setAtBit(flags,PACKET_IS_COLOR_BIT);
    }
    if (_currentPacketIsCompressed) {
        setAtBit(flags,PACKET_IS_COMPRESSED_BIT);
    }

    _particlePacketAvailableBytes = MAX_PACKET_SIZE;
    int numBytesPacketHeader = populateTypeAndVersion(_particlePacket, PACKET_TYPE_PARTICLE_DATA);
    _particlePacketAt = _particlePacket + numBytesPacketHeader;
    _particlePacketAvailableBytes -= numBytesPacketHeader;

    // pack in flags
    PARTICLE_PACKET_FLAGS* flagsAt = (PARTICLE_PACKET_FLAGS*)_particlePacketAt;
    *flagsAt = flags;
    _particlePacketAt += sizeof(PARTICLE_PACKET_FLAGS);
    _particlePacketAvailableBytes -= sizeof(PARTICLE_PACKET_FLAGS);

    // pack in sequence number
    PARTICLE_PACKET_SEQUENCE* sequenceAt = (PARTICLE_PACKET_SEQUENCE*)_particlePacketAt;
    *sequenceAt = _sequenceNumber;
    _particlePacketAt += sizeof(PARTICLE_PACKET_SEQUENCE);
    _particlePacketAvailableBytes -= sizeof(PARTICLE_PACKET_SEQUENCE);
    if (!(lastWasSurpressed || _lastParticlePacketLength == PARTICLE_PACKET_HEADER_SIZE)) {
        _sequenceNumber++;
    }

    // pack in timestamp
    PARTICLE_PACKET_SENT_TIME now = usecTimestampNow();
    PARTICLE_PACKET_SENT_TIME* timeAt = (PARTICLE_PACKET_SENT_TIME*)_particlePacketAt;
    *timeAt = now;
    _particlePacketAt += sizeof(PARTICLE_PACKET_SENT_TIME);
    _particlePacketAvailableBytes -= sizeof(PARTICLE_PACKET_SENT_TIME);

    _particlePacketWaiting = false;
}

void ParticleReceiverData::writeToPacket(const unsigned char* buffer, int bytes) {
    // compressed packets include lead bytes which contain compressed size, this allows packing of
    // multiple compressed portions together
    if (_currentPacketIsCompressed) {
        *(PARTICLE_PACKET_INTERNAL_SECTION_SIZE*)_particlePacketAt = bytes;
        _particlePacketAt += sizeof(PARTICLE_PACKET_INTERNAL_SECTION_SIZE);
        _particlePacketAvailableBytes -= sizeof(PARTICLE_PACKET_INTERNAL_SECTION_SIZE);
    }
    if (bytes <= _particlePacketAvailableBytes) {
        memcpy(_particlePacketAt, buffer, bytes);
        _particlePacketAvailableBytes -= bytes;
        _particlePacketAt += bytes;
        _particlePacketWaiting = true;
    }    
}

ParticleReceiverData::~ParticleReceiverData() {
    delete[] _particlePacket;
    delete[] _lastParticlePacket;

    if (_particleSendThread) {
        _particleSendThread->terminate();
        delete _particleSendThread;
    }
}

bool ParticleReceiverData::updateCurrentViewFrustum() {
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
        if (_lastClientParticleSizeScale != getParticleSizeScale()) {
            _lastClientParticleSizeScale = getParticleSizeScale();
            _lodChanged = true;
        }
    } else {
        _lodInitialized = true;
        _lastClientParticleSizeScale = getParticleSizeScale();
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

void ParticleReceiverData::setViewSent(bool viewSent) { 
    _viewSent = viewSent; 
    if (viewSent) {
        _viewFrustumJustStoppedChanging = false;
        _lodChanged = false;
    }
}

void ParticleReceiverData::updateLastKnownViewFrustum() {
    bool frustumChanges = !_lastKnownViewFrustum.isVerySimilar(_currentViewFrustum);
    
    if (frustumChanges) {
        // save our currentViewFrustum into our lastKnownViewFrustum
        _lastKnownViewFrustum = _currentViewFrustum;
    }
    
    // save that we know the view has been sent.
    uint64_t now = usecTimestampNow();
    setLastTimeBagEmpty(now); // is this what we want? poor names
}


bool ParticleReceiverData::moveShouldDump() const {
    glm::vec3 oldPosition = _lastKnownViewFrustum.getPosition();
    glm::vec3 newPosition = _currentViewFrustum.getPosition();

    // theoretically we could make this slightly larger but relative to avatar scale.    
    const float MAXIMUM_MOVE_WITHOUT_DUMP = 0.0f;
    if (glm::distance(newPosition, oldPosition) > MAXIMUM_MOVE_WITHOUT_DUMP) {
        return true;
    }
    return false;
}

void ParticleReceiverData::dumpOutOfView() {
    int stillInView = 0;
    int outOfView = 0;
    ParticleNodeBag tempBag;
    while (!nodeBag.isEmpty()) {
        ParticleNode* node = nodeBag.extract();
        if (node->isInView(_currentViewFrustum)) {
            tempBag.insert(node);
            stillInView++;
        } else {
            outOfView++;
        }
    }
    if (stillInView > 0) {
        while (!tempBag.isEmpty()) {
            ParticleNode* node = tempBag.extract();
            if (node->isInView(_currentViewFrustum)) {
                nodeBag.insert(node);
            }
        }
    }
}

