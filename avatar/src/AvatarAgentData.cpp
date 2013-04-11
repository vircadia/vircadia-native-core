//
//  AvatarAgentData.cpp
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//
//

#include <cstdio>

#include "AvatarAgentData.h"

AvatarAgentData::AvatarAgentData() {
    
}

AvatarAgentData::~AvatarAgentData() {
    
}

AvatarAgentData* AvatarAgentData::clone() const {
    return new AvatarAgentData(*this);
}

void AvatarAgentData::parseData(void *data, int size) {
    char* packetData = (char *)data + 1;
    
    // Extract data from packet
    sscanf(packetData,
           PACKET_FORMAT,
           &_pitch,
           &_yaw,
           &_roll,
           &_headPositionX,
           &_headPositionY,
           &_headPositionZ,
           &_loudness,
           &_averageLoudness,
           &_handPositionX,
           &_handPositionY,
           &_handPositionZ);
}

float AvatarAgentData::getPitch() {
    return _pitch;
}

float AvatarAgentData::getYaw() {
    return _yaw;
}

float AvatarAgentData::getRoll() {
    return _roll;
}

float AvatarAgentData::getHeadPositionX() {
    return _headPositionX;
}

float AvatarAgentData::getHeadPositionY() {
    return _headPositionY;
}

float AvatarAgentData::getHeadPositionZ() {
    return _headPositionZ;
}

float AvatarAgentData::getLoudness() {
    return _loudness;
}

float AvatarAgentData::getAverageLoudness() {
    return _averageLoudness;
}

float AvatarAgentData::getHandPositionX() {
    return _handPositionX;
}

float AvatarAgentData::getHandPositionY() {
    return _handPositionY;
}

float AvatarAgentData::getHandPositionZ() {
    return _handPositionZ;
}

void AvatarAgentData::setPitch(float pitch) {
    _pitch = pitch;
}

void AvatarAgentData::setYaw(float yaw) {
    _yaw = yaw;
}

void AvatarAgentData::setRoll(float roll) {
    _roll = roll;
}

void AvatarAgentData::setHeadPosition(float x, float y, float z) {
    _headPositionX = x;
    _headPositionY = y;
    _headPositionZ = z;
}

void AvatarAgentData::setLoudness(float loudness) {
    _loudness = loudness;
}

void AvatarAgentData::setAverageLoudness(float averageLoudness) {
    _averageLoudness = averageLoudness;
}

void AvatarAgentData::setHandPosition(float x, float y, float z) {
    _handPositionX = x;
    _handPositionY = y;
    _handPositionZ = z;
}
