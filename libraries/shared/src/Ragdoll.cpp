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

Ragdoll::Ragdoll() : _ragdollTranslation(0.0f), _translationInSimulationFrame(0.0f), _ragdollSimulation(NULL) {
}

Ragdoll::~Ragdoll() {
    clearRagdollConstraintsAndPoints();
}

void Ragdoll::stepRagdollForward(float deltaTime) {
    if (_ragdollSimulation) {
        updateSimulationTransforms(_ragdollTranslation - _ragdollSimulation->getTranslation(), _ragdollRotation);
    }
    int numPoints = _ragdollPoints.size();
    for (int i = 0; i < numPoints; ++i) {
        _ragdollPoints[i].integrateForward();
    }
}
    
void Ragdoll::clearRagdollConstraintsAndPoints() {
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
    _ragdollPoints.clear();
}

float Ragdoll::enforceRagdollConstraints() {
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

void Ragdoll::initRagdollTransform() {
    _ragdollTranslation = glm::vec3(0.0f);
    _ragdollRotation = glm::quat();
    _translationInSimulationFrame = glm::vec3(0.0f);
    _rotationInSimulationFrame = glm::quat();
}

void Ragdoll::setRagdollTransform(const glm::vec3& translation, const glm::quat& rotation) {
    _ragdollTranslation = translation;
    _ragdollRotation = rotation;
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
    int numPoints = _ragdollPoints.size();
    for (int i = 0; i < numPoints; ++i) {
        _ragdollPoints[i].move(deltaPosition, deltaRotation, _translationInSimulationFrame);
    }

    // remember the current transform
    _translationInSimulationFrame = translation;
    _rotationInSimulationFrame = rotation;
}
