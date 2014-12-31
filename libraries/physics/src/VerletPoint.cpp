//
//  VerletPoint.cpp
//  libraries/physics/src
//
//  Created by Andrew Meadows 2014.07.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VerletPoint.h"

const float INTEGRATION_FRICTION_FACTOR = 0.6f;

void VerletPoint::integrateForward() {
    glm::vec3 oldPosition = _position;
    _position += INTEGRATION_FRICTION_FACTOR * (_position - _lastPosition);
    _lastPosition = oldPosition;
}

void VerletPoint::accumulateDelta(const glm::vec3& delta) {
    _accumulatedDelta += delta;
    ++_numDeltas;
}

void VerletPoint::applyAccumulatedDelta() {
    if (_numDeltas > 0) { 
        _position += _accumulatedDelta / (float)_numDeltas;
        _accumulatedDelta = glm::vec3(0.0f);
        _numDeltas = 0;
    }
}

void VerletPoint::move(const glm::vec3& deltaPosition, const glm::quat& deltaRotation, const glm::vec3& oldPivot) {
    glm::vec3 arm = _position - oldPivot;
    _position += deltaPosition + (deltaRotation * arm - arm);
    arm = _lastPosition - oldPivot;
    _lastPosition += deltaPosition + (deltaRotation * arm - arm);
}

void VerletPoint::shift(const glm::vec3& deltaPosition) {
    _position += deltaPosition;
    _lastPosition += deltaPosition;
}

void VerletPoint::setMass(float mass) {
    const float MIN_MASS = 1.0e-6f;
    const float MAX_MASS = 1.0e18f;
    if (glm::isnan(mass)) {
        mass = MIN_MASS;
    }
    _mass = glm::clamp(glm::abs(mass), MIN_MASS, MAX_MASS);
}
