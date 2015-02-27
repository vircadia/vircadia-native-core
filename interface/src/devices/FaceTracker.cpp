//
//  FaceTracker.cpp
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 4/9/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FaceTracker.h"

inline float FaceTracker::getBlendshapeCoefficient(int index) const {
    return isValidBlendshapeIndex(index) ? _blendshapeCoefficients[index] : 0.0f;
}

float FaceTracker::getFadeCoefficient() const {
    return _fadeCoefficient;
}

const glm::vec3 FaceTracker::getHeadTranslation() const {
    return glm::mix(glm::vec3(0.0f), _headTranslation, getFadeCoefficient());
}

const glm::quat FaceTracker::getHeadRotation() const {
    return glm::mix(glm::quat(), _headRotation, getFadeCoefficient());
}

void FaceTracker::update(float deltaTime) {
    static const float RELAXATION_TIME = 1.0f; // sec
    static const float LAMBDA = 1.5;
    
    if (isTracking()) {
        if (_relaxationStatus == 1.0f) {
            return;
        }
        _relaxationStatus = glm::clamp(_relaxationStatus + deltaTime / RELAXATION_TIME, 0.0f, 1.0f);
        _fadeCoefficient = std::exp(-LAMBDA * _relaxationStatus);
    } else {
        if (_relaxationStatus == 0.0f) {
            return;
        }
        _relaxationStatus = glm::clamp(_relaxationStatus - deltaTime / RELAXATION_TIME, 0.0f, 1.0f);
        _fadeCoefficient = 1.0f - std::exp(-LAMBDA * (1.0f - _relaxationStatus));
    }
}