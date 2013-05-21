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

void HeadData::addYaw(float yaw) {
    setYaw(_yaw + yaw);
}

void HeadData::addPitch(float pitch) {
    setPitch(_pitch + pitch);
}

void HeadData::addRoll(float roll) {
    setRoll(_roll + roll);
}