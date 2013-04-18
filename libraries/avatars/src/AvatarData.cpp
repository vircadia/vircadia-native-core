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
    _bodyYaw(-90.0),
    _bodyPitch(0.0),
    _bodyRoll(0.0) {
    
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
    
    memcpy(destinationBuffer, &_bodyPosition, sizeof(float) * 3);
    destinationBuffer += sizeof(float) * 3;
    
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyYaw);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyPitch);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyRoll);
    
    //printf( "_bodyYaw = %f", _bodyYaw );
    
    memcpy(destinationBuffer, &_handPosition, sizeof(float) * 3);
    destinationBuffer += sizeof(float) * 3;
    
    //std::cout << _bodyPosition.x << ", " << _bodyPosition.y << ", " << _bodyPosition.z << "\n";
    
    return destinationBuffer - bufferStart;
}

// called on the other agents - assigns it to my views of the others
void AvatarData::parseData(unsigned char* sourceBuffer, int numBytes) {
    // increment to push past the packet header
    sourceBuffer++;
    
    memcpy(&_bodyPosition, sourceBuffer, sizeof(float) * 3);
    sourceBuffer += sizeof(float) * 3;
   
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t *)sourceBuffer, &_bodyYaw);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t *)sourceBuffer, &_bodyPitch);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t *)sourceBuffer, &_bodyRoll);

    memcpy(&_handPosition, sourceBuffer, sizeof(float) * 3);
    sourceBuffer += sizeof(float) * 3;

    //std::cout << _bodyPosition.x << ", " << _bodyPosition.y << ", " << _bodyPosition.z << "\n";


}

glm::vec3 AvatarData::getBodyPosition() {
    return glm::vec3(_bodyPosition.x,
                     _bodyPosition.y,
                     _bodyPosition.z);
}

void AvatarData::setBodyPosition(glm::vec3 bodyPosition) {
    _bodyPosition = bodyPosition;
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


