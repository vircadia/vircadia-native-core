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
    // Based on exponential distributions: http://en.wikipedia.org/wiki/Exponential_distribution
    static const float EPSILON = 0.02f;
    static const float LAMBDA = 1.5;
    static const float INVERSE_AT_EPSILON = -std::log(EPSILON) / LAMBDA; // So that f(1) = EPSILON ~ 0
    static const float RELAXATION_TIME = 0.8f; // sec
    
    if (isTracking()) {
        if (_relaxationStatus == 1.0f) {
            _fadeCoefficient = 1.0f;
            return;
        }
        _relaxationStatus = glm::clamp(_relaxationStatus + deltaTime / RELAXATION_TIME, 0.0f, 1.0f);
        _fadeCoefficient = 1.0f - std::exp(-LAMBDA * _relaxationStatus * INVERSE_AT_EPSILON);
    } else {
        if (_relaxationStatus == 0.0f) {
            _fadeCoefficient = 0.0f;
            return;
        }
        _relaxationStatus = glm::clamp(_relaxationStatus - deltaTime / RELAXATION_TIME, 0.0f, 1.0f);
        _fadeCoefficient = std::exp(-LAMBDA * (1.0f - _relaxationStatus) * INVERSE_AT_EPSILON);
    }
}