//
//  OctreeQuery.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 10/24/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstdio>
#include <cstring>
#include <stdint.h>

#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>
#include "OctreeConstants.h"

#include "OctreeQuery.h"

using namespace std;

static const float fingerVectorRadix = 4; // bits of precision when converting from float<->fixed

OctreeQuery::OctreeQuery(Node* owningNode) :
    NodeData(owningNode),
    _uuid(),
    _cameraPosition(0,0,0),
    _cameraOrientation(),
    _cameraFov(0.0f),
    _cameraAspectRatio(0.0f),
    _cameraNearClip(0.0f),
    _cameraFarClip(0.0f),
    _wantColor(true),
    _wantDelta(true),
    _wantLowResMoving(true),
    _wantOcclusionCulling(false), // disabled by default
    _wantCompression(false), // disabled by default
    _maxOctreePPS(DEFAULT_MAX_OCTREE_PPS),
    _octreeElementSizeScale(DEFAULT_OCTREE_SIZE_SCALE)
{
    
}

OctreeQuery::~OctreeQuery() {
    // nothing to do
}

int OctreeQuery::getBroadcastData(unsigned char* destinationBuffer) {
    unsigned char* bufferStart = destinationBuffer;
    
    // TODO: DRY this up to a shared method
    // that can pack any type given the number of bytes
    // and return the number of bytes to push the pointer
    
    // UUID
    QByteArray uuidByteArray = _uuid.toRfc4122();
    memcpy(destinationBuffer, uuidByteArray.constData(), uuidByteArray.size());
    destinationBuffer += uuidByteArray.size();
    
    // camera details
    memcpy(destinationBuffer, &_cameraPosition, sizeof(_cameraPosition));
    destinationBuffer += sizeof(_cameraPosition);
    destinationBuffer += packOrientationQuatToBytes(destinationBuffer, _cameraOrientation);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _cameraFov);
    destinationBuffer += packFloatRatioToTwoByte(destinationBuffer, _cameraAspectRatio);
    destinationBuffer += packClipValueToTwoByte(destinationBuffer, _cameraNearClip);
    destinationBuffer += packClipValueToTwoByte(destinationBuffer, _cameraFarClip);
    memcpy(destinationBuffer, &_cameraEyeOffsetPosition, sizeof(_cameraEyeOffsetPosition));
    destinationBuffer += sizeof(_cameraEyeOffsetPosition);

    // bitMask of less than byte wide items
    unsigned char bitItems = 0;
    if (_wantLowResMoving)     { setAtBit(bitItems, WANT_LOW_RES_MOVING_BIT); }
    if (_wantColor)            { setAtBit(bitItems, WANT_COLOR_AT_BIT); }
    if (_wantDelta)            { setAtBit(bitItems, WANT_DELTA_AT_BIT); }
    if (_wantOcclusionCulling) { setAtBit(bitItems, WANT_OCCLUSION_CULLING_BIT); }
    if (_wantCompression)      { setAtBit(bitItems, WANT_COMPRESSION); }

    *destinationBuffer++ = bitItems;

    // desired Max Octree PPS
    memcpy(destinationBuffer, &_maxOctreePPS, sizeof(_maxOctreePPS));
    destinationBuffer += sizeof(_maxOctreePPS);

    // desired voxelSizeScale
    memcpy(destinationBuffer, &_octreeElementSizeScale, sizeof(_octreeElementSizeScale));
    destinationBuffer += sizeof(_octreeElementSizeScale);

    // desired boundaryLevelAdjust
    memcpy(destinationBuffer, &_boundaryLevelAdjust, sizeof(_boundaryLevelAdjust));
    destinationBuffer += sizeof(_boundaryLevelAdjust);
    
    return destinationBuffer - bufferStart;
}

// called on the other nodes - assigns it to my views of the others
int OctreeQuery::parseData(unsigned char* sourceBuffer, int numBytes) {

    // increment to push past the packet header
    int numBytesPacketHeader = numBytesForPacketHeader(sourceBuffer);
    sourceBuffer += numBytesPacketHeader;
    
    unsigned char* startPosition = sourceBuffer;
    
    // push past the node session UUID
    sourceBuffer += NUM_BYTES_RFC4122_UUID;
    
    // user UUID
    _uuid = QUuid::fromRfc4122(QByteArray((char*) sourceBuffer, NUM_BYTES_RFC4122_UUID));
    sourceBuffer += NUM_BYTES_RFC4122_UUID;
    
    // camera details
    memcpy(&_cameraPosition, sourceBuffer, sizeof(_cameraPosition));
    sourceBuffer += sizeof(_cameraPosition);
    sourceBuffer += unpackOrientationQuatFromBytes(sourceBuffer, _cameraOrientation);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &_cameraFov);
    sourceBuffer += unpackFloatRatioFromTwoByte(sourceBuffer,_cameraAspectRatio);
    sourceBuffer += unpackClipValueFromTwoByte(sourceBuffer,_cameraNearClip);
    sourceBuffer += unpackClipValueFromTwoByte(sourceBuffer,_cameraFarClip);
    memcpy(&_cameraEyeOffsetPosition, sourceBuffer, sizeof(_cameraEyeOffsetPosition));
    sourceBuffer += sizeof(_cameraEyeOffsetPosition);

    // voxel sending features...
    unsigned char bitItems = 0;
    bitItems = (unsigned char)*sourceBuffer++;
    _wantLowResMoving = oneAtBit(bitItems, WANT_LOW_RES_MOVING_BIT);
    _wantColor = oneAtBit(bitItems, WANT_COLOR_AT_BIT);
    _wantDelta = oneAtBit(bitItems, WANT_DELTA_AT_BIT);
    _wantOcclusionCulling = oneAtBit(bitItems, WANT_OCCLUSION_CULLING_BIT);
    _wantCompression = oneAtBit(bitItems, WANT_COMPRESSION);

    // desired Max Octree PPS
    memcpy(&_maxOctreePPS, sourceBuffer, sizeof(_maxOctreePPS));
    sourceBuffer += sizeof(_maxOctreePPS);

    // desired _octreeElementSizeScale
    memcpy(&_octreeElementSizeScale, sourceBuffer, sizeof(_octreeElementSizeScale));
    sourceBuffer += sizeof(_octreeElementSizeScale);

    // desired boundaryLevelAdjust
    memcpy(&_boundaryLevelAdjust, sourceBuffer, sizeof(_boundaryLevelAdjust));
    sourceBuffer += sizeof(_boundaryLevelAdjust);

    return sourceBuffer - startPosition;
}

glm::vec3 OctreeQuery::calculateCameraDirection() const {
    glm::vec3 direction = glm::vec3(_cameraOrientation * glm::vec4(IDENTITY_FRONT, 0.0f));
    return direction;
}

