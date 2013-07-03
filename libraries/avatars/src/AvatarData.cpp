//
//  AvatarData.cpp
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstdio>
#include <cstring>
#include <stdint.h>

#include <SharedUtil.h>
#include <PacketHeaders.h>

#include "AvatarData.h"
#include <VoxelConstants.h>

using namespace std;

AvatarData::AvatarData(Agent* owningAgent) :
    AgentData(owningAgent),
    _handPosition(0,0,0),
    _bodyYaw(-90.0),
    _bodyPitch(0.0),
    _bodyRoll(0.0),
    _handState(0),
    _cameraPosition(0,0,0),
    _cameraOrientation(),
    _cameraFov(0.0f),
    _cameraAspectRatio(0.0f),
    _cameraNearClip(0.0f),
    _cameraFarClip(0.0f),
    _keyState(NO_KEY_DOWN),
    _wantResIn(false),
    _wantColor(true),
    _wantDelta(false),
    _wantOcclusionCulling(false),
    _headData(NULL),
    _handData(NULL)
{
    
}

AvatarData::~AvatarData() {
    delete _headData;
    delete _handData;
}

int AvatarData::getBroadcastData(unsigned char* destinationBuffer) {
    unsigned char* bufferStart = destinationBuffer;
    
    // TODO: DRY this up to a shared method
    // that can pack any type given the number of bytes
    // and return the number of bytes to push the pointer
    
    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_headData) {
        _headData = new HeadData(this);
    }
    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_handData) {
        _handData = new HandData(this);
    }
    
    // Body world position
    memcpy(destinationBuffer, &_position, sizeof(float) * 3);
    destinationBuffer += sizeof(float) * 3;
    
    // Body rotation (NOTE: This needs to become a quaternion to save two bytes)
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyYaw);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyPitch);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyRoll);
    
    // Head rotation (NOTE: This needs to become a quaternion to save two bytes)
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _headData->_yaw);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _headData->_pitch);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _headData->_roll);
    
    // Head lean X,Z (head lateral and fwd/back motion relative to torso)
    memcpy(destinationBuffer, &_headData->_leanSideways, sizeof(_headData->_leanSideways));
    destinationBuffer += sizeof(_headData->_leanSideways);
    memcpy(destinationBuffer, &_headData->_leanForward, sizeof(_headData->_leanForward));
    destinationBuffer += sizeof(_headData->_leanForward);

    // Hand Position - is relative to body position
    glm::vec3 handPositionRelative = _handPosition - _position;
    memcpy(destinationBuffer, &handPositionRelative, sizeof(float) * 3);
    destinationBuffer += sizeof(float) * 3;

    // Lookat Position
    memcpy(destinationBuffer, &_headData->_lookAtPosition, sizeof(_headData->_lookAtPosition));
    destinationBuffer += sizeof(_headData->_lookAtPosition);
     
    // Instantaneous audio loudness (used to drive facial animation)
    //destinationBuffer += packFloatToByte(destinationBuffer, std::min(MAX_AUDIO_LOUDNESS, _audioLoudness), MAX_AUDIO_LOUDNESS);
    memcpy(destinationBuffer, &_headData->_audioLoudness, sizeof(float));
    destinationBuffer += sizeof(float); 

    // camera details
    memcpy(destinationBuffer, &_cameraPosition, sizeof(_cameraPosition));
    destinationBuffer += sizeof(_cameraPosition);
    destinationBuffer += packOrientationQuatToBytes(destinationBuffer, _cameraOrientation);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _cameraFov);
    destinationBuffer += packFloatRatioToTwoByte(destinationBuffer, _cameraAspectRatio);
    destinationBuffer += packClipValueToTwoByte(destinationBuffer, _cameraNearClip);
    destinationBuffer += packClipValueToTwoByte(destinationBuffer, _cameraFarClip);

    // chat message
    *destinationBuffer++ = _chatMessage.size();
    memcpy(destinationBuffer, _chatMessage.data(), _chatMessage.size() * sizeof(char));
    destinationBuffer += _chatMessage.size() * sizeof(char);
    
    // bitMask of less than byte wide items
    unsigned char bitItems = 0;
    if (_wantResIn) { setAtBit(bitItems, WANT_RESIN_AT_BIT); }
    if (_wantColor) { setAtBit(bitItems, WANT_COLOR_AT_BIT); }
    if (_wantDelta) { setAtBit(bitItems, WANT_DELTA_AT_BIT); }
    if (_wantOcclusionCulling) { setAtBit(bitItems, WANT_OCCLUSION_CULLING_BIT); }

    // key state
    setSemiNibbleAt(bitItems,KEY_STATE_START_BIT,_keyState);
    // hand state
    setSemiNibbleAt(bitItems,HAND_STATE_START_BIT,_handState);
    *destinationBuffer++ = bitItems;
    
    // leap hand data
    // In order to make the hand data version-robust, hand data packing is just a series of vec3's,
    // with conventions. If a client doesn't know the conventions, they can just get the vec3's
    // and render them as balls, or ignore them, without crashing or disrupting anyone.
    // Current convention:
    //    Zero or more fingetTip positions, followed by the same number of fingerRoot positions
    
    const std::vector<glm::vec3>& fingerTips = _handData->getFingerTips();
    const std::vector<glm::vec3>& fingerRoots = _handData->getFingerRoots();
    size_t numFingerVectors = fingerTips.size() + fingerRoots.size();
    if (numFingerVectors > 255)
        numFingerVectors = 0; // safety. We shouldn't ever get over 255, so consider that invalid.

    *destinationBuffer++ = (unsigned char)numFingerVectors;
    
    if (numFingerVectors > 0) {
        for (size_t i = 0; i < fingerTips.size(); ++i) {
            destinationBuffer += packFloatScalarToSignedTwoByteFixed(destinationBuffer, fingerTips[i].x, 4);
            destinationBuffer += packFloatScalarToSignedTwoByteFixed(destinationBuffer, fingerTips[i].y, 4);
            destinationBuffer += packFloatScalarToSignedTwoByteFixed(destinationBuffer, fingerTips[i].z, 4);
        }
        for (size_t i = 0; i < fingerRoots.size(); ++i) {
            destinationBuffer += packFloatScalarToSignedTwoByteFixed(destinationBuffer, fingerRoots[i].x, 4);
            destinationBuffer += packFloatScalarToSignedTwoByteFixed(destinationBuffer, fingerRoots[i].y, 4);
            destinationBuffer += packFloatScalarToSignedTwoByteFixed(destinationBuffer, fingerRoots[i].z, 4);
        }
    }
    
    return destinationBuffer - bufferStart;
}

// called on the other agents - assigns it to my views of the others
int AvatarData::parseData(unsigned char* sourceBuffer, int numBytes) {

    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_headData) {
        _headData = new HeadData(this);
    }
    
    // lazily allocate memory for HandData in case we're not an Avatar instance
    if (!_handData) {
        _handData = new HandData(this);
    }
    
    // increment to push past the packet header
    sourceBuffer += sizeof(PACKET_HEADER_HEAD_DATA);
    
    unsigned char* startPosition = sourceBuffer;
    
    // push past the agent ID
    sourceBuffer += + sizeof(uint16_t);
    
    // Body world position
    memcpy(&_position, sourceBuffer, sizeof(float) * 3);
    sourceBuffer += sizeof(float) * 3;
    
    // Body rotation (NOTE: This needs to become a quaternion to save two bytes)
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &_bodyYaw);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &_bodyPitch);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &_bodyRoll);
    
    // Head rotation (NOTE: This needs to become a quaternion to save two bytes)
    float headYaw, headPitch, headRoll;
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &headYaw);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &headPitch);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &headRoll);
    
    _headData->setYaw(headYaw);
    _headData->setPitch(headPitch);
    _headData->setRoll(headRoll);

    //  Head position relative to pelvis
    memcpy(&_headData->_leanSideways, sourceBuffer, sizeof(_headData->_leanSideways));
    sourceBuffer += sizeof(float);
    memcpy(&_headData->_leanForward, sourceBuffer, sizeof(_headData->_leanForward));
    sourceBuffer += sizeof(_headData->_leanForward);

    // Hand Position - is relative to body position
    glm::vec3 handPositionRelative;
    memcpy(&handPositionRelative, sourceBuffer, sizeof(float) * 3);
    _handPosition = _position + handPositionRelative;
    sourceBuffer += sizeof(float) * 3;
    
    // Lookat Position
    memcpy(&_headData->_lookAtPosition, sourceBuffer, sizeof(_headData->_lookAtPosition));
    sourceBuffer += sizeof(_headData->_lookAtPosition);

    // Instantaneous audio loudness (used to drive facial animation)
    //sourceBuffer += unpackFloatFromByte(sourceBuffer, _audioLoudness, MAX_AUDIO_LOUDNESS);
    memcpy(&_headData->_audioLoudness, sourceBuffer, sizeof(float));
    sourceBuffer += sizeof(float);
    
    // camera details
    memcpy(&_cameraPosition, sourceBuffer, sizeof(_cameraPosition));
    sourceBuffer += sizeof(_cameraPosition);
    sourceBuffer += unpackOrientationQuatFromBytes(sourceBuffer, _cameraOrientation);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &_cameraFov);
    sourceBuffer += unpackFloatRatioFromTwoByte(sourceBuffer,_cameraAspectRatio);
    sourceBuffer += unpackClipValueFromTwoByte(sourceBuffer,_cameraNearClip);
    sourceBuffer += unpackClipValueFromTwoByte(sourceBuffer,_cameraFarClip);

    // the rest is a chat message
    int chatMessageSize = *sourceBuffer++;
    _chatMessage = string((char*)sourceBuffer, chatMessageSize);
    sourceBuffer += chatMessageSize * sizeof(char);

    // voxel sending features...
    unsigned char bitItems = 0;
    bitItems = (unsigned char)*sourceBuffer++;
    _wantResIn = oneAtBit(bitItems, WANT_RESIN_AT_BIT);
    _wantColor = oneAtBit(bitItems, WANT_COLOR_AT_BIT);
    _wantDelta = oneAtBit(bitItems, WANT_DELTA_AT_BIT);
    _wantOcclusionCulling = oneAtBit(bitItems, WANT_OCCLUSION_CULLING_BIT);

    // key state, stored as a semi-nibble in the bitItems
    _keyState = (KeyState)getSemiNibbleAt(bitItems,KEY_STATE_START_BIT);

    // hand state, stored as a semi-nibble in the bitItems
    _handState = getSemiNibbleAt(bitItems,HAND_STATE_START_BIT);

    // leap hand data
    if (sourceBuffer - startPosition < numBytes)    // safety check
    {
        std::vector<glm::vec3> fingerTips = _handData->getFingerTips();
        std::vector<glm::vec3> fingerRoots = _handData->getFingerRoots();
        unsigned int numFingerVectors = *sourceBuffer++;
        unsigned int numFingerTips = numFingerVectors / 2;
        unsigned int numFingerRoots = numFingerVectors - numFingerTips;
        fingerTips.resize(numFingerTips);
        fingerRoots.resize(numFingerRoots);
        for (size_t i = 0; i < numFingerTips; ++i) {
            sourceBuffer += unpackFloatScalarFromSignedTwoByteFixed((int16_t*) sourceBuffer, &(fingerTips[i].x), 4);
            sourceBuffer += unpackFloatScalarFromSignedTwoByteFixed((int16_t*) sourceBuffer, &(fingerTips[i].y), 4);
            sourceBuffer += unpackFloatScalarFromSignedTwoByteFixed((int16_t*) sourceBuffer, &(fingerTips[i].z), 4);
        }
        _handData->setFingerTips(fingerTips);
        _handData->setFingerRoots(fingerRoots);
    }
    
    return sourceBuffer - startPosition;
}

glm::vec3 AvatarData::calculateCameraDirection() const {
    glm::vec3 direction = glm::vec3(_cameraOrientation * glm::vec4(IDENTITY_FRONT, 0.0f));
    return direction;
}


// Allows sending of fixed-point numbers: radix 1 makes 15.1 number, radix 8 makes 8.8 number, etc
int packFloatScalarToSignedTwoByteFixed(unsigned char* buffer, float scalar, int radix) {
    int16_t outVal = (int16_t)(scalar * (float)(1 << radix));
    memcpy(buffer, &outVal, sizeof(uint16_t));
    return sizeof(uint16_t);
}

int unpackFloatScalarFromSignedTwoByteFixed(int16_t* byteFixedPointer, float* destinationPointer, int radix) {
    *destinationPointer = *byteFixedPointer / (float)(1 << radix);
    return sizeof(int16_t);
}

int packFloatAngleToTwoByte(unsigned char* buffer, float angle) {
    const float ANGLE_CONVERSION_RATIO = (std::numeric_limits<uint16_t>::max() / 360.0);
    
    uint16_t angleHolder = floorf((angle + 180) * ANGLE_CONVERSION_RATIO);
    memcpy(buffer, &angleHolder, sizeof(uint16_t));
    
    return sizeof(uint16_t);
}

int unpackFloatAngleFromTwoByte(uint16_t* byteAnglePointer, float* destinationPointer) {
    *destinationPointer = (*byteAnglePointer / (float) std::numeric_limits<uint16_t>::max()) * 360.0 - 180;
    return sizeof(uint16_t);
}

int packOrientationQuatToBytes(unsigned char* buffer, const glm::quat& quatInput) {
    const float QUAT_PART_CONVERSION_RATIO = (std::numeric_limits<uint16_t>::max() / 2.0);
    uint16_t quatParts[4];
    quatParts[0] = floorf((quatInput.x + 1.0) * QUAT_PART_CONVERSION_RATIO);
    quatParts[1] = floorf((quatInput.y + 1.0) * QUAT_PART_CONVERSION_RATIO);
    quatParts[2] = floorf((quatInput.z + 1.0) * QUAT_PART_CONVERSION_RATIO);
    quatParts[3] = floorf((quatInput.w + 1.0) * QUAT_PART_CONVERSION_RATIO);

    memcpy(buffer, &quatParts, sizeof(quatParts));
    return sizeof(quatParts);
}

int unpackOrientationQuatFromBytes(unsigned char* buffer, glm::quat& quatOutput) {
    uint16_t quatParts[4];
    memcpy(&quatParts, buffer, sizeof(quatParts));

    quatOutput.x = ((quatParts[0] / (float) std::numeric_limits<uint16_t>::max()) * 2.0) - 1.0;
    quatOutput.y = ((quatParts[1] / (float) std::numeric_limits<uint16_t>::max()) * 2.0) - 1.0;
    quatOutput.z = ((quatParts[2] / (float) std::numeric_limits<uint16_t>::max()) * 2.0) - 1.0;
    quatOutput.w = ((quatParts[3] / (float) std::numeric_limits<uint16_t>::max()) * 2.0) - 1.0;

    return sizeof(quatParts);
}

float SMALL_LIMIT = 10.0;
float LARGE_LIMIT = 1000.0;

int packFloatRatioToTwoByte(unsigned char* buffer, float ratio) {
    // if the ratio is less than 10, then encode it as a positive number scaled from 0 to int16::max()
    int16_t ratioHolder;
    
    if (ratio < SMALL_LIMIT) {
        const float SMALL_RATIO_CONVERSION_RATIO = (std::numeric_limits<int16_t>::max() / SMALL_LIMIT);
        ratioHolder = floorf(ratio * SMALL_RATIO_CONVERSION_RATIO);
    } else {
        const float LARGE_RATIO_CONVERSION_RATIO = std::numeric_limits<int16_t>::min() / LARGE_LIMIT;
        ratioHolder = floorf((std::min(ratio,LARGE_LIMIT) - SMALL_LIMIT) * LARGE_RATIO_CONVERSION_RATIO);
    }
    memcpy(buffer, &ratioHolder, sizeof(ratioHolder));
    return sizeof(ratioHolder);
}

int unpackFloatRatioFromTwoByte(unsigned char* buffer, float& ratio) {
    int16_t ratioHolder;
    memcpy(&ratioHolder, buffer, sizeof(ratioHolder));

    // If it's positive, than the original ratio was less than SMALL_LIMIT
    if (ratioHolder > 0) {
        ratio = (ratioHolder / (float) std::numeric_limits<int16_t>::max()) * SMALL_LIMIT;
    } else {
        // If it's negative, than the original ratio was between SMALL_LIMIT and LARGE_LIMIT
        ratio = ((ratioHolder / (float) std::numeric_limits<int16_t>::min()) * LARGE_LIMIT) + SMALL_LIMIT;
    }
    return sizeof(ratioHolder);
}

int packClipValueToTwoByte(unsigned char* buffer, float clipValue) {
    // Clip values must be less than max signed 16bit integers
    assert(clipValue < std::numeric_limits<int16_t>::max());
    int16_t holder;
    
    // if the clip is less than 10, then encode it as a positive number scaled from 0 to int16::max()
    if (clipValue < SMALL_LIMIT) {
        const float SMALL_RATIO_CONVERSION_RATIO = (std::numeric_limits<int16_t>::max() / SMALL_LIMIT);
        holder = floorf(clipValue * SMALL_RATIO_CONVERSION_RATIO);
    } else {
        // otherwise we store it as a negative integer
        holder = -1 * floorf(clipValue);
    }
    memcpy(buffer, &holder, sizeof(holder));
    return sizeof(holder);
}

int unpackClipValueFromTwoByte(unsigned char* buffer, float& clipValue) {
    int16_t holder;
    memcpy(&holder, buffer, sizeof(holder));

    // If it's positive, than the original clipValue was less than SMALL_LIMIT
    if (holder > 0) {
        clipValue = (holder / (float) std::numeric_limits<int16_t>::max()) * SMALL_LIMIT;
    } else {
        // If it's negative, than the original holder can be found as the opposite sign of holder
        clipValue = -1.0f * holder;
    }
    return sizeof(holder);
}

int packFloatToByte(unsigned char* buffer, float value, float scaleBy) {
    unsigned char holder;
    const float CONVERSION_RATIO = (255 / scaleBy);
    holder = floorf(value * CONVERSION_RATIO);
    memcpy(buffer, &holder, sizeof(holder));
    return sizeof(holder);
}

int unpackFloatFromByte(unsigned char* buffer, float& value, float scaleBy) {
    unsigned char holder;
    memcpy(&holder, buffer, sizeof(holder));
    value = ((float)holder / (float) 255) * scaleBy;
    return sizeof(holder);
}

