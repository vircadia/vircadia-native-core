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

#include "Ragdoll.h"

#include "CapsuleShape.h"
#include "CollisionInfo.h"
#include "SharedUtil.h"
#include "SphereShape.h"

// ----------------------------------------------------------------------------
// FixedConstraint
// ----------------------------------------------------------------------------
FixedConstraint::FixedConstraint(glm::vec3* point, const glm::vec3& anchor) : _point(point), _anchor(anchor) {
}

float FixedConstraint::enforce() {
    assert(_point != NULL);
    float distance = glm::distance(_anchor, *_point);
    *_point = _anchor;
    return distance;
}

void FixedConstraint::setPoint(glm::vec3* point) {
    _point = point;
}

void FixedConstraint::setAnchor(const glm::vec3& anchor) {
    _anchor = anchor;
}

// ----------------------------------------------------------------------------
// DistanceConstraint
// ----------------------------------------------------------------------------
DistanceConstraint::DistanceConstraint(VerletPoint* startPoint, VerletPoint* endPoint) : _distance(-1.0f) {
    _points[0] = startPoint;
    _points[1] = endPoint;
    _distance = glm::distance(_points[0]->_position, _points[1]->_position);
}

DistanceConstraint::DistanceConstraint(const DistanceConstraint& other) {
    _distance = other._distance;
    _points[0] = other._points[0];
    _points[1] = other._points[1];
}

void DistanceConstraint::setDistance(float distance) {
    _distance = fabsf(distance);
}

float DistanceConstraint::enforce() {
    // TODO: use a fast distance approximation
    float newDistance = glm::distance(_points[0]->_position, _points[1]->_position);
    glm::vec3 direction(0.0f, 1.0f, 0.0f);
    if (newDistance > EPSILON) {
        direction = (_points[0]->_position - _points[1]->_position) / newDistance;
    }
    glm::vec3 center = 0.5f * (_points[0]->_position + _points[1]->_position);
    _points[0]->_position = center + (0.5f * _distance) * direction;
    _points[1]->_position = center - (0.5f * _distance) * direction;
    return glm::abs(newDistance - _distance);
}

// ----------------------------------------------------------------------------
// Ragdoll
// ----------------------------------------------------------------------------

Ragdoll::Ragdoll() {
}

Ragdoll::~Ragdoll() {
    clearRagdollConstraintsAndPoints();
}
    
void Ragdoll::clearRagdollConstraintsAndPoints() {
    int numConstraints = _ragdollConstraints.size();
    for (int i = 0; i < numConstraints; ++i) {
        delete _ragdollConstraints[i];
    }
    _ragdollConstraints.clear();
    _ragdollPoints.clear();
}

float Ragdoll::enforceRagdollConstraints() {
    float maxDistance = 0.0f;
    const int numConstraints = _ragdollConstraints.size();
    for (int i = 0; i < numConstraints; ++i) {
        DistanceConstraint* c = static_cast<DistanceConstraint*>(_ragdollConstraints[i]);
        //maxDistance = glm::max(maxDistance, _ragdollConstraints[i]->enforce());
        maxDistance = glm::max(maxDistance, c->enforce());
    }
    return maxDistance;
}

