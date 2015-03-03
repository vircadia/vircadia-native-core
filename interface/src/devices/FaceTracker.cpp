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

#include <GLMHelpers.h>

#include "FaceTracker.h"

inline float FaceTracker::getBlendshapeCoefficient(int index) const {
    return isValidBlendshapeIndex(index) ? glm::mix(0.0f, _blendshapeCoefficients[index], getFadeCoefficient())
                                         : 0.0f;
}

const QVector<float>& FaceTracker::getBlendshapeCoefficients() const {
    static QVector<float> blendshapes;
    float fadeCoefficient = getFadeCoefficient();
    if (fadeCoefficient == 1.0f) {
        return _blendshapeCoefficients;
    } else {
        blendshapes.resize(_blendshapeCoefficients.size());
        for (int i = 0; i < _blendshapeCoefficients.size(); i++) {
            blendshapes[i] = glm::mix(0.0f, _blendshapeCoefficients[i], fadeCoefficient);
        }
        return blendshapes;
    }
}

float FaceTracker::getFadeCoefficient() const {
    return _fadeCoefficient;
}

const glm::vec3 FaceTracker::getHeadTranslation() const {
    return glm::mix(glm::vec3(0.0f), _headTranslation, getFadeCoefficient());
}

const glm::quat FaceTracker::getHeadRotation() const {
    return safeMix(glm::quat(), _headRotation, getFadeCoefficient());
}

void FaceTracker::update(float deltaTime) {
    // Based on exponential distributions: http://en.wikipedia.org/wiki/Exponential_distribution
    static const float EPSILON = 0.02f; // MUST BE < 1.0f
    static const float INVERSE_AT_EPSILON = -std::log(EPSILON); // So that f(1.0f) = EPSILON ~ 0.0f
    static const float RELAXATION_TIME = 0.8f; // sec
    
    if (isTracking()) {
        if (_relaxationStatus == 1.0f) {
            _fadeCoefficient = 1.0f;
            return;
        }
        _relaxationStatus = glm::clamp(_relaxationStatus + deltaTime / RELAXATION_TIME, 0.0f, 1.0f);
        _fadeCoefficient = 1.0f - std::exp(-_relaxationStatus * INVERSE_AT_EPSILON);
    } else {
        if (_relaxationStatus == 0.0f) {
            _fadeCoefficient = 0.0f;
            return;
        }
        _relaxationStatus = glm::clamp(_relaxationStatus - deltaTime / RELAXATION_TIME, 0.0f, 1.0f);
        _fadeCoefficient = std::exp(-(1.0f - _relaxationStatus) * INVERSE_AT_EPSILON);
    }
}