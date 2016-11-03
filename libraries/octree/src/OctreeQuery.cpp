//
//  OctreeQuery.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 10/24/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <GLMHelpers.h>
#include <udt/PacketHeaders.h>

#include "OctreeConstants.h"
#include "OctreeQuery.h"


OctreeQuery::OctreeQuery() {
    _maxQueryPPS = DEFAULT_MAX_OCTREE_PPS;
}

int OctreeQuery::getBroadcastData(unsigned char* destinationBuffer) {
    unsigned char* bufferStart = destinationBuffer;
    
    // TODO: DRY this up to a shared method
    // that can pack any type given the number of bytes
    // and return the number of bytes to push the pointer
    
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

    // NOTE: we need to keep these here for new clients to talk to old servers. After we know that the clients and
    // servers and clients have all been updated we could remove these bits. New servers will always force these
    // features on old clients even if they don't ask for them. (which old clients will properly handle). New clients
    // will always ask for these so that old servers will use these features.
    setAtBit(bitItems, WANT_LOW_RES_MOVING_BIT);
    setAtBit(bitItems, WANT_COLOR_AT_BIT);
    setAtBit(bitItems, WANT_DELTA_AT_BIT);
    setAtBit(bitItems, WANT_COMPRESSION);

    *destinationBuffer++ = bitItems;

    // desired Max Octree PPS
    memcpy(destinationBuffer, &_maxQueryPPS, sizeof(_maxQueryPPS));
    destinationBuffer += sizeof(_maxQueryPPS);

    // desired voxelSizeScale
    memcpy(destinationBuffer, &_octreeElementSizeScale, sizeof(_octreeElementSizeScale));
    destinationBuffer += sizeof(_octreeElementSizeScale);

    // desired boundaryLevelAdjust
    memcpy(destinationBuffer, &_boundaryLevelAdjust, sizeof(_boundaryLevelAdjust));
    destinationBuffer += sizeof(_boundaryLevelAdjust);

    memcpy(destinationBuffer, &_cameraCenterRadius, sizeof(_cameraCenterRadius));
    destinationBuffer += sizeof(_cameraCenterRadius);
    
    return destinationBuffer - bufferStart;
}

// called on the other nodes - assigns it to my views of the others
int OctreeQuery::parseData(ReceivedMessage& message) {
 
    const unsigned char* startPosition = reinterpret_cast<const unsigned char*>(message.getRawMessage());
    const unsigned char* sourceBuffer = startPosition;
    
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

    // optional feature flags
    unsigned char bitItems = 0;
    bitItems = (unsigned char)*sourceBuffer++;

    // NOTE: we used to use these bits to set feature request items if we need to extend the protocol with optional features
    // do it here with... wantFeature= oneAtBit(bitItems, WANT_FEATURE_BIT);
    Q_UNUSED(bitItems);

    // desired Max Octree PPS
    memcpy(&_maxQueryPPS, sourceBuffer, sizeof(_maxQueryPPS));
    sourceBuffer += sizeof(_maxQueryPPS);

    // desired _octreeElementSizeScale
    memcpy(&_octreeElementSizeScale, sourceBuffer, sizeof(_octreeElementSizeScale));
    sourceBuffer += sizeof(_octreeElementSizeScale);

    // desired boundaryLevelAdjust
    memcpy(&_boundaryLevelAdjust, sourceBuffer, sizeof(_boundaryLevelAdjust));
    sourceBuffer += sizeof(_boundaryLevelAdjust);

    auto bytesRead = sourceBuffer - startPosition;
    auto bytesLeft = message.getSize() - bytesRead;
    if (bytesLeft >= (int)sizeof(_cameraCenterRadius)) {
        memcpy(&_cameraCenterRadius, sourceBuffer, sizeof(_cameraCenterRadius));
        sourceBuffer += sizeof(_cameraCenterRadius);
    }
    return sourceBuffer - startPosition;
}

glm::vec3 OctreeQuery::calculateCameraDirection() const {
    glm::vec3 direction = glm::vec3(_cameraOrientation * glm::vec4(IDENTITY_FRONT, 0.0f));
    return direction;
}
