//
//  HeadData.cpp
//  hifi
//
//  Created by Stephen Birarda on 5/20/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "HeadData.h"

HeadData::HeadData(AvatarData* owningAvatar) :
    _yaw(0.0f),
    _pitch(0.0f),
    _roll(0.0f),
    _lookAtPosition(0.0f, 0.0f, 0.0f),
    _leanSideways(0.0f),
    _leanForward(0.0f),
    _owningAvatar(owningAvatar)
{
    
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


void HeadData::addLean(float sideways, float forwards) {
    // Add lean as impulse
    _leanSideways += sideways;
    _leanForward  += forwards;
}