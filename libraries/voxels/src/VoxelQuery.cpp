//
//  VoxelQuery.cpp
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
#include "VoxelConstants.h"

#include "VoxelQuery.h"

using namespace std;

static const float fingerVectorRadix = 4; // bits of precision when converting from float<->fixed

VoxelQuery::VoxelQuery(Node* owningNode) :
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
    _wantOcclusionCulling(true),
    _maxVoxelPPS(DEFAULT_MAX_VOXEL_PPS),
    _voxelSizeScale(DEFAULT_VOXEL_SIZE_SCALE)
{
    
}

VoxelQuery::~VoxelQuery() {
}

int VoxelQuery::getBroadcastData(unsigned char* destinationBuffer) {
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

    *destinationBuffer++ = bitItems;

    // desired Max Voxel PPS
    memcpy(destinationBuffer, &_maxVoxelPPS, sizeof(_maxVoxelPPS));
    destinationBuffer += sizeof(_maxVoxelPPS);

    // desired voxelSizeScale
    memcpy(destinationBuffer, &_voxelSizeScale, sizeof(_voxelSizeScale));
    destinationBuffer += sizeof(_voxelSizeScale);

    // desired boundaryLevelAdjust
    memcpy(destinationBuffer, &_boundaryLevelAdjust, sizeof(_boundaryLevelAdjust));
    destinationBuffer += sizeof(_boundaryLevelAdjust);
    
    return destinationBuffer - bufferStart;
}

// called on the other nodes - assigns it to my views of the others
int VoxelQuery::parseData(unsigned char* sourceBuffer, int numBytes) {

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
    _wantLowResMoving     = oneAtBit(bitItems, WANT_LOW_RES_MOVING_BIT);
    _wantColor            = oneAtBit(bitItems, WANT_COLOR_AT_BIT);
    _wantDelta            = oneAtBit(bitItems, WANT_DELTA_AT_BIT);
    _wantOcclusionCulling = oneAtBit(bitItems, WANT_OCCLUSION_CULLING_BIT);

    // desired Max Voxel PPS
    memcpy(&_maxVoxelPPS, sourceBuffer, sizeof(_maxVoxelPPS));
    sourceBuffer += sizeof(_maxVoxelPPS);

    // desired voxelSizeScale
    memcpy(&_voxelSizeScale, sourceBuffer, sizeof(_voxelSizeScale));
    sourceBuffer += sizeof(_voxelSizeScale);

    // desired boundaryLevelAdjust
    memcpy(&_boundaryLevelAdjust, sourceBuffer, sizeof(_boundaryLevelAdjust));
    sourceBuffer += sizeof(_boundaryLevelAdjust);

    return sourceBuffer - startPosition;
}

glm::vec3 VoxelQuery::calculateCameraDirection() const {
    glm::vec3 direction = glm::vec3(_cameraOrientation * glm::vec4(IDENTITY_FRONT, 0.0f));
    return direction;
}

