//
//  HeadData.cpp
//  hifi
//
//  Created by Stephen Birarda on 5/20/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "HeadData.h"

HeadData::HeadData() :
    _yaw(0.0f),
    _pitch(0.0f),
    _roll(0.0f),
    _lookAtPosition(0.0f, 0.0f, 0.0f)
{
    
}

void HeadData::setYaw(float yaw) {
    const float MAX_YAW = 85;
    const float MIN_YAW = -85;
    
    _yaw = glm::clamp(yaw, MIN_YAW, MAX_YAW);
}

void HeadData::setPitch(float pitch) {
    // set head pitch and apply limits
    const float MAX_PITCH = 60;
    const float MIN_PITCH = -60;
    
    _pitch = glm::clamp(pitch, MIN_PITCH, MAX_PITCH);
}

void HeadData::setRoll(float roll) {
    const float MAX_ROLL = 50;
    const float MIN_ROLL = -50;
    
    _roll = glm::clamp(roll, MIN_ROLL, MAX_ROLL);
}

void HeadData::addYaw(float yaw) {
    setYaw(_yaw + yaw);
}

void HeadData::addPitch(float pitch) {
    setPitch(_pitch + pitch);
}

void HeadData::addRoll(float roll) {
    setRoll(_roll + roll);
}