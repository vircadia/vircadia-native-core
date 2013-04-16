//
//  AvatarData.cpp
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//
//

#include <cstdio>

#include <PacketHeaders.h>

#include "AvatarData.h"

int packFloatAngleToTwoByte(char* buffer, float angle) {
    const float ANGLE_CONVERSION_RATIO = (std::numeric_limits<uint16_t>::max() / 360.0);
    
    uint16_t angleHolder = floorf((angle + 180) * ANGLE_CONVERSION_RATIO);
    memcpy(buffer, &angleHolder, sizeof(uint16_t));
    
    return sizeof(uint16_t);
}

int unpackFloatAngleFromTwoByte(uint16_t *byteAnglePointer, float *destinationPointer) {
    *destinationPointer = (*byteAnglePointer / std::numeric_limits<uint16_t>::max()) * 360.0 - 180;
    return sizeof(uint16_t);
}

AvatarData::AvatarData() :
    _bodyYaw(-90.0),
    _bodyPitch(0.0),
    _bodyRoll(0.0) {
    
}

AvatarData::~AvatarData() {
    
}

AvatarData* AvatarData::clone() const {
    return new AvatarData(*this);
}

// transmit data to agents requesting it
// called on me just prior to sending data to others (continuasly called)
int AvatarData::getBroadcastData(char* destinationBuffer) {
    char* bufferPointer = destinationBuffer;
    *(bufferPointer++) = PACKET_HEADER_HEAD_DATA;

    // TODO: DRY this up to a shared method
    // that can pack any type given the number of bytes
    // and return the number of bytes to push the pointer
    memcpy(bufferPointer, &_bodyPosition, sizeof(float) * 3);
    bufferPointer += sizeof(float) * 3;
    
    bufferPointer += packFloatAngleToTwoByte(bufferPointer, _bodyYaw);
    bufferPointer += packFloatAngleToTwoByte(bufferPointer, _bodyPitch);
    bufferPointer += packFloatAngleToTwoByte(bufferPointer, _bodyRoll);
    
    return bufferPointer - destinationBuffer;
}

// called on the other agents - assigns it to my views of the others
void AvatarData::parseData(void *sourceBuffer, int numBytes) {
    
    char* bufferPointer = (char*) sourceBuffer + 1;
    
    memcpy(&_bodyPosition, bufferPointer, sizeof(float) * 3);
    bufferPointer += sizeof(float) * 3;
   
    bufferPointer += unpackFloatAngleFromTwoByte((uint16_t*)bufferPointer, &_bodyYaw);
    bufferPointer += unpackFloatAngleFromTwoByte((uint16_t*)bufferPointer, &_bodyPitch);
    bufferPointer += unpackFloatAngleFromTwoByte((uint16_t*)bufferPointer, &_bodyRoll);
}

glm::vec3 AvatarData::getBodyPosition() {
    return glm::vec3(_bodyPosition.x,
                     _bodyPosition.y,
                     _bodyPosition.z);
}

void AvatarData::setBodyPosition(glm::vec3 bodyPosition) {
    _bodyPosition = bodyPosition;
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


