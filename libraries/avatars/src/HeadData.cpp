//
//  HeadData.cpp
//  libraries/avatars/src
//
//  Created by Stephen Birarda on 5/20/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <FaceshiftConstants.h>
#include <GLMHelpers.h>

#include "AvatarData.h"
#include "HeadData.h"

/// The names of the blendshapes expected by Faceshift, terminated with an empty string.
extern const char* FACESHIFT_BLENDSHAPES[];
/// The size of FACESHIFT_BLENDSHAPES
extern const int NUM_FACESHIFT_BLENDSHAPES;

HeadData::HeadData(AvatarData* owningAvatar) :
    _baseYaw(0.0f),
    _basePitch(0.0f),
    _baseRoll(0.0f),
    _leanSideways(0.0f),
    _leanForward(0.0f),
    _torsoTwist(0.0f),
    _lookAtPosition(0.0f, 0.0f, 0.0f),
    _audioLoudness(0.0f),
    _isFaceTrackerConnected(false),
    _isEyeTrackerConnected(false),
    _leftEyeBlink(0.0f),
    _rightEyeBlink(0.0f),
    _averageLoudness(0.0f),
    _browAudioLift(0.0f),
    _owningAvatar(owningAvatar)
{
    
}

glm::quat HeadData::getOrientation() const {
    return _owningAvatar->getOrientation() * glm::quat(glm::radians(glm::vec3(_basePitch, _baseYaw, _baseRoll)));
}

void HeadData::setOrientation(const glm::quat& orientation) {
    // rotate body about vertical axis
    glm::quat bodyOrientation = _owningAvatar->getOrientation();
    glm::vec3 newFront = glm::inverse(bodyOrientation) * (orientation * IDENTITY_FRONT);
    bodyOrientation = bodyOrientation * glm::angleAxis(atan2f(-newFront.x, -newFront.z), glm::vec3(0.0f, 1.0f, 0.0f));
    _owningAvatar->setOrientation(bodyOrientation);
    
    // the rest goes to the head
    glm::vec3 eulers = glm::degrees(safeEulerAngles(glm::inverse(bodyOrientation) * orientation));
    _basePitch = eulers.x;
    _baseYaw = eulers.y;
    _baseRoll = eulers.z;
}

void HeadData::setBlendshape(QString name, float val) {
    static bool hasInitializedLookupMap = false;
    static QMap<QString, int> blendshapeLookupMap;
    //Lazily construct a lookup map from the blendshapes
    if (!hasInitializedLookupMap) {
        for (int i = 0; i < NUM_FACESHIFT_BLENDSHAPES; i++) {
            blendshapeLookupMap[FACESHIFT_BLENDSHAPES[i]] = i; 
        }
        hasInitializedLookupMap = true;
    }

    //Check to see if the named blendshape exists, and then set its value if it does
    QMap<QString, int>::iterator it = blendshapeLookupMap.find(name);
    if (it != blendshapeLookupMap.end()) {
        if (_blendshapeCoefficients.size() <= it.value()) {
            _blendshapeCoefficients.resize(it.value() + 1);
        }
        _blendshapeCoefficients[it.value()] = val;
    }
}
