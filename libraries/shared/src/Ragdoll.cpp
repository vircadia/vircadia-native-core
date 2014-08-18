//
//  Ragdoll.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.05.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/norm.hpp>

#include "Ragdoll.h"

#include "Constraint.h"
#include "DistanceConstraint.h"
#include "FixedConstraint.h"
#include "PhysicsSimulation.h"
#include "SharedUtil.h" // for EPSILON

Ragdoll::Ragdoll() : _massScale(1.0f), _translation(0.0f), _translationInSimulationFrame(0.0f), _simulation(NULL) {
}

Ragdoll::~Ragdoll() {
    clearConstraintsAndPoints();
    if (_simulation) {
        _simulation->removeRagdoll(this);
    }
}

void Ragdoll::stepForward(float deltaTime) {
    if (_simulation) {
        updateSimulationTransforms(_translation - _simulation->getTranslation(), _rotation);
    }
    int numPoints = _points.size();
    for (int i = 0; i < numPoints; ++i) {
        _points[i].integrateForward();
    }
}
    
void Ragdoll::clearConstraintsAndPoints() {
    int numConstraints = _boneConstraints.size();
    for (int i = 0; i < numConstraints; ++i) {
        delete _boneConstraints[i];
    }
    _boneConstraints.clear();
    numConstraints = _fixedConstraints.size();
    for (int i = 0; i < numConstraints; ++i) {
        delete _fixedConstraints[i];
    }
    _fixedConstraints.clear();
    _points.clear();
}

float Ragdoll::enforceConstraints() {
    float maxDistance = 0.0f;
    // enforce the bone constraints first
    int numConstraints = _boneConstraints.size();
    for (int i = 0; i < numConstraints; ++i) {
        maxDistance = glm::max(maxDistance, _boneConstraints[i]->enforce());
    }
    // enforce FixedConstraints second
    numConstraints = _fixedConstraints.size();
    for (int i = 0; i < _fixedConstraints.size(); ++i) {
        maxDistance = glm::max(maxDistance, _fixedConstraints[i]->enforce());
    }
    return maxDistance;
}

void Ragdoll::initTransform() {
    _translation = glm::vec3(0.0f);
    _rotation = glm::quat();
    _translationInSimulationFrame = glm::vec3(0.0f);
    _rotationInSimulationFrame = glm::quat();
}

void Ragdoll::setTransform(const glm::vec3& translation, const glm::quat& rotation) {
    _translation = translation;
    _rotation = rotation;
}

void Ragdoll::updateSimulationTransforms(const glm::vec3& translation, const glm::quat& rotation) {
    const float EPSILON2 = EPSILON * EPSILON;
    if (glm::distance2(translation, _translationInSimulationFrame) < EPSILON2 && 
            glm::abs(1.0f - glm::abs(glm::dot(rotation, _rotationInSimulationFrame))) < EPSILON2) {
        // nothing to do
        return;
    }

    // compute linear and angular deltas
    glm::vec3 deltaPosition = translation - _translationInSimulationFrame;
    glm::quat deltaRotation = rotation * glm::inverse(_rotationInSimulationFrame);

    // apply the deltas to all ragdollPoints
    int numPoints = _points.size();
    for (int i = 0; i < numPoints; ++i) {
        _points[i].move(deltaPosition, deltaRotation, _translationInSimulationFrame);
    }

    // remember the current transform
    _translationInSimulationFrame = translation;
    _rotationInSimulationFrame = rotation;
}

void Ragdoll::setMassScale(float scale) {
    const float MIN_SCALE = 1.0e-2f;
    const float MAX_SCALE = 1.0e6f;
    scale = glm::clamp(glm::abs(scale), MIN_SCALE, MAX_SCALE);
    if (scale != _massScale) {
        float rescale = scale / _massScale;
        int numPoints = _points.size();
        for (int i = 0; i < numPoints; ++i) {
            _points[i].setMass(rescale * _points[i].getMass());
        }
        _massScale = scale;
    }
}
