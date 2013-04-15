//
//  AvatarData.cpp
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//
//

#include <cstdio>

#include "AvatarData.h"

AvatarData::AvatarData() {
    
}

AvatarData::~AvatarData() {
    
}

AvatarData* AvatarData::clone() const {
    return new AvatarData(*this);
}

void AvatarData::parseData(void *data, int size) {
    char* packetData = (char *)data + 1;
    
//    // Extract data from packet
//    sscanf(packetData,
//           PACKET_FORMAT,
//           &_pitch,
//           &_yaw,
//           &_roll,
//           &_headPositionX,
//           &_headPositionY,
//           &_headPositionZ,
//           &_loudness,
//           &_averageLoudness,
//           &_handPositionX,
//           &_handPositionY,
//           &_handPositionZ);
}

float AvatarData::getPitch() {
    return _pitch;
}

float AvatarData::getYaw() {
    return _yaw;
}

float AvatarData::getRoll() {
    return _roll;
}

float AvatarData::getHeadPositionX() {
    return _headPositionX;
}

float AvatarData::getHeadPositionY() {
    return _headPositionY;
}

float AvatarData::getHeadPositionZ() {
    return _headPositionZ;
}

float AvatarData::getLoudness() {
    return _loudness;
}

float AvatarData::getAverageLoudness() {
    return _averageLoudness;
}

float AvatarData::getHandPositionX() {
    return _handPositionX;
}

float AvatarData::getHandPositionY() {
    return _handPositionY;
}

float AvatarData::getHandPositionZ() {
    return _handPositionZ;
}

void AvatarData::setPitch(float pitch) {
    _pitch = pitch;
}

void AvatarData::setYaw(float yaw) {
    _yaw = yaw;
}

void AvatarData::setRoll(float roll) {
    _roll = roll;
}

void AvatarData::setHeadPosition(float x, float y, float z) {
    _headPositionX = x;
    _headPositionY = y;
    _headPositionZ = z;
}

void AvatarData::setLoudness(float loudness) {
    _loudness = loudness;
}

void AvatarData::setAverageLoudness(float averageLoudness) {
    _averageLoudness = averageLoudness;
}

void AvatarData::setHandPosition(float x, float y, float z) {
    _handPositionX = x;
    _handPositionY = y;
    _handPositionZ = z;
}
