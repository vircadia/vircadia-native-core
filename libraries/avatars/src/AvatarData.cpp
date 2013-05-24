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

using namespace std;

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

AvatarData::AvatarData(Agent* owningAgent) :
    AgentData(owningAgent),
    _handPosition(0,0,0),
    _bodyYaw(-90.0),
    _bodyPitch(0.0),
    _bodyRoll(0.0),
    _audioLoudness(0),
    _handState(0),
    _cameraPosition(0,0,0),
    _cameraDirection(0,0,0),
    _cameraUp(0,0,0),
    _cameraRight(0,0,0),
    _cameraFov(0.0f),
    _cameraAspectRatio(0.0f),
    _cameraNearClip(0.0f),
    _cameraFarClip(0.0f),
    _keyState(NO_KEY_DOWN),
    _wantResIn(false),
    _wantColor(true),
    _wantDelta(false),
    _headData(NULL)
{
}

AvatarData::~AvatarData() {
    delete _headData;
}

int AvatarData::getBroadcastData(unsigned char* destinationBuffer) {
    unsigned char* bufferStart = destinationBuffer;
    
    // TODO: DRY this up to a shared method
    // that can pack any type given the number of bytes
    // and return the number of bytes to push the pointer
    
    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_headData) {
        _headData = new HeadData();
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

    // Hand Position
    memcpy(destinationBuffer, &_handPosition, sizeof(float) * 3);
    destinationBuffer += sizeof(float) * 3;

    // Lookat Position
    memcpy(destinationBuffer, &_headData->_lookAtPosition, sizeof(_headData->_lookAtPosition));
    destinationBuffer += sizeof(_headData->_lookAtPosition);
     
    // Hand State (0 = not grabbing, 1 = grabbing)
    memcpy(destinationBuffer, &_handState, sizeof(char));
    destinationBuffer += sizeof(char);
    
    // Instantaneous audio loudness (used to drive facial animation)
    memcpy(destinationBuffer, &_audioLoudness, sizeof(float));
    destinationBuffer += sizeof(float); 

    // camera details
    memcpy(destinationBuffer, &_cameraPosition, sizeof(_cameraPosition));
    destinationBuffer += sizeof(_cameraPosition);
    memcpy(destinationBuffer, &_cameraDirection, sizeof(_cameraDirection));
    destinationBuffer += sizeof(_cameraDirection);
    memcpy(destinationBuffer, &_cameraRight, sizeof(_cameraRight));
    destinationBuffer += sizeof(_cameraRight);
    memcpy(destinationBuffer, &_cameraUp, sizeof(_cameraUp));
    destinationBuffer += sizeof(_cameraUp);
    memcpy(destinationBuffer, &_cameraFov, sizeof(_cameraFov));
    destinationBuffer += sizeof(_cameraFov);
    memcpy(destinationBuffer, &_cameraAspectRatio, sizeof(_cameraAspectRatio));
    destinationBuffer += sizeof(_cameraAspectRatio);
    memcpy(destinationBuffer, &_cameraNearClip, sizeof(_cameraNearClip));
    destinationBuffer += sizeof(_cameraNearClip);
    memcpy(destinationBuffer, &_cameraFarClip, sizeof(_cameraFarClip));
    destinationBuffer += sizeof(_cameraFarClip);

    // key state
    *destinationBuffer++ = _keyState;

    // chat message
    *destinationBuffer++ = _chatMessage.size();
    memcpy(destinationBuffer, _chatMessage.data(), _chatMessage.size() * sizeof(char));
    destinationBuffer += _chatMessage.size() * sizeof(char);
    
    // voxel sending features...
    // voxel sending features...
    unsigned char wantItems = 0;
    if (_wantResIn) { setAtBit(wantItems,WANT_RESIN_AT_BIT); }
    if (_wantColor) { setAtBit(wantItems,WANT_COLOR_AT_BIT); }
    if (_wantDelta) { setAtBit(wantItems,WANT_DELTA_AT_BIT); }
    *destinationBuffer++ = wantItems;
    
    return destinationBuffer - bufferStart;
}

// called on the other agents - assigns it to my views of the others
int AvatarData::parseData(unsigned char* sourceBuffer, int numBytes) {

    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_headData) {
        _headData = new HeadData();
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

    // Hand Position
    memcpy(&_handPosition, sourceBuffer, sizeof(float) * 3);
    sourceBuffer += sizeof(float) * 3;
    
    // Lookat Position
    memcpy(&_headData->_lookAtPosition, sourceBuffer, sizeof(_headData->_lookAtPosition));
    sourceBuffer += sizeof(_headData->_lookAtPosition);
        
    // Hand State
    memcpy(&_handState, sourceBuffer, sizeof(char));
    sourceBuffer += sizeof(char);
    
    // Instantaneous audio loudness (used to drive facial animation)
    memcpy(&_audioLoudness, sourceBuffer, sizeof(float));
    sourceBuffer += sizeof(float);
    
    // camera details
    memcpy(&_cameraPosition, sourceBuffer, sizeof(_cameraPosition));
    sourceBuffer += sizeof(_cameraPosition);
    memcpy(&_cameraDirection, sourceBuffer, sizeof(_cameraDirection));
    sourceBuffer += sizeof(_cameraDirection);
    memcpy(&_cameraRight, sourceBuffer, sizeof(_cameraRight));
    sourceBuffer += sizeof(_cameraRight);
    memcpy(&_cameraUp, sourceBuffer, sizeof(_cameraUp));
    sourceBuffer += sizeof(_cameraUp);
    memcpy(&_cameraFov, sourceBuffer, sizeof(_cameraFov));
    sourceBuffer += sizeof(_cameraFov);
    memcpy(&_cameraAspectRatio, sourceBuffer, sizeof(_cameraAspectRatio));
    sourceBuffer += sizeof(_cameraAspectRatio);
    memcpy(&_cameraNearClip, sourceBuffer, sizeof(_cameraNearClip));
    sourceBuffer += sizeof(_cameraNearClip);
    memcpy(&_cameraFarClip, sourceBuffer, sizeof(_cameraFarClip));
    sourceBuffer += sizeof(_cameraFarClip);
    
    // key state
    _keyState = (KeyState)*sourceBuffer++;
    
    // the rest is a chat message
    int chatMessageSize = *sourceBuffer++;
    _chatMessage = string((char*)sourceBuffer, chatMessageSize);
    sourceBuffer += chatMessageSize * sizeof(char);

    // voxel sending features...
    unsigned char wantItems = 0;
    wantItems = (unsigned char)*sourceBuffer++;
    _wantResIn = oneAtBit(wantItems,WANT_RESIN_AT_BIT);
    _wantColor = oneAtBit(wantItems,WANT_COLOR_AT_BIT);
    _wantDelta = oneAtBit(wantItems,WANT_DELTA_AT_BIT);

    return sourceBuffer - startPosition;
}
