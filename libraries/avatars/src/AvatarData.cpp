//
//  AvatarData.cpp
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//
//

#include <cstdio>
#include <cstring>
#include <stdint.h>

#include <SharedUtil.h>
#include <PacketHeaders.h>

#include "AvatarData.h"
#include "avatars_Log.h"

using namespace std;
using avatars_lib::printLog;


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

AvatarData::AvatarData() :
    _handPosition(0,0,0),
    _bodyYaw(-90.0),
    _bodyPitch(0.0),
    _bodyRoll(0.0),
    _headYaw(0),
    _headPitch(0),
    _headRoll(0),
    _handState(0),
    _cameraPosition(0,0,0),
    _cameraDirection(0,0,0),
    _cameraUp(0,0,0),
    _cameraRight(0,0,0),
    _cameraFov(0.0f),
    _cameraAspectRatio(0.0f),
    _cameraNearClip(0.0f),
    _cameraFarClip(0.0f),
    _keyState(NO_KEY_DOWN) {
    
}

AvatarData::~AvatarData() {
    
}

AvatarData* AvatarData::clone() const {
    return new AvatarData(*this);
}

int AvatarData::getBroadcastData(unsigned char* destinationBuffer) {
    unsigned char* bufferStart = destinationBuffer;
    
    // TODO: DRY this up to a shared method
    // that can pack any type given the number of bytes
    // and return the number of bytes to push the pointer
    
    // Body world position
    memcpy(destinationBuffer, &_position, sizeof(float) * 3);
    destinationBuffer += sizeof(float) * 3;
    
    // Body rotation (NOTE: This needs to become a quaternion to save two bytes)
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyYaw);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyPitch);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyRoll);
    
    // Head rotation (NOTE: This needs to become a quaternion to save two bytes)
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _headYaw);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _headPitch);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _headRoll);
    
    // Hand Position 
    memcpy(destinationBuffer, &_handPosition, sizeof(float) * 3);
    destinationBuffer += sizeof(float) * 3;
    
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
    
    return destinationBuffer - bufferStart;
}

// called on the other agents - assigns it to my views of the others
int AvatarData::parseData(unsigned char* sourceBuffer, int numBytes) {

    // increment to push past the packet header and agent ID
    sourceBuffer += sizeof(PACKET_HEADER_HEAD_DATA) + sizeof(uint16_t);
    
    unsigned char* startPosition = sourceBuffer;
    
    // Body world position
    memcpy(&_position, sourceBuffer, sizeof(float) * 3);
    sourceBuffer += sizeof(float) * 3;
   
    // Body rotation (NOTE: This needs to become a quaternion to save two bytes)
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t *)sourceBuffer, &_bodyYaw);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t *)sourceBuffer, &_bodyPitch);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t *)sourceBuffer, &_bodyRoll);
    
    // Head rotation (NOTE: This needs to become a quaternion to save two bytes)
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t *)sourceBuffer, &_headYaw);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t *)sourceBuffer, &_headPitch);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t *)sourceBuffer, &_headRoll);

    // Hand Position
    memcpy(&_handPosition, sourceBuffer, sizeof(float) * 3);
    sourceBuffer += sizeof(float) * 3;
    
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
    
    return sourceBuffer - startPosition;
}

const glm::vec3& AvatarData::getPosition() const {
    return _position;
}

void AvatarData::setPosition(glm::vec3 position) {
    _position = position;
}

void AvatarData::setHandPosition(glm::vec3 handPosition) {
    _handPosition = handPosition;
}

float AvatarData::getBodyYaw() {
    return _bodyYaw;
}

void AvatarData::setBodyYaw(float bodyYaw) {
    _bodyYaw = bodyYaw;
}

float AvatarData::getBodyPitch() {
    return _bodyPitch;
}

void AvatarData::setBodyPitch(float bodyPitch) {
    _bodyPitch = bodyPitch;
}

float AvatarData::getBodyRoll() {
    return _bodyRoll;
}

void AvatarData::setBodyRoll(float bodyRoll) {
    _bodyRoll = bodyRoll;
}


