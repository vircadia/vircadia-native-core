//
//  HeadData.cpp
//  hifi
//
//  Created by Stephen Birarda on 5/20/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <glm/gtx/quaternion.hpp>

#include <OctreeConstants.h>

#include "AvatarData.h"
#include "HeadData.h"

HeadData::HeadData(AvatarData* owningAvatar) :
    _yaw(0.0f),
    _pitch(0.0f),
    _roll(0.0f),
    _leanSideways(0.0f),
    _leanForward(0.0f),
    _lookAtPosition(0.0f, 0.0f, 0.0f),
    _audioLoudness(0.0f),
    _isFaceshiftConnected(false),
    _leftEyeBlink(0.0f),
    _rightEyeBlink(0.0f),
    _averageLoudness(0.0f),
    _browAudioLift(0.0f),
    _owningAvatar(owningAvatar)
{
    
}

glm::quat HeadData::getOrientation() const {
    return _owningAvatar->getOrientation() * glm::quat(glm::radians(glm::vec3(_pitch, _yaw, _roll)));
}

void HeadData::setOrientation(const glm::quat& orientation) {
    // rotate body about vertical axis
    glm::quat bodyOrientation = _owningAvatar->getOrientation();
    glm::vec3 newFront = glm::inverse(bodyOrientation) * (orientation * IDENTITY_FRONT);
    bodyOrientation = bodyOrientation * glm::angleAxis(atan2f(-newFront.x, -newFront.z), glm::vec3(0.0f, 1.0f, 0.0f));
    _owningAvatar->setOrientation(bodyOrientation);
    
    // the rest goes to the head
    glm::vec3 eulers = DEGREES_PER_RADIAN * safeEulerAngles(glm::inverse(bodyOrientation) * orientation);
    _pitch = eulers.x;
    _yaw = eulers.y;
    _roll = eulers.z;
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

