//
//  RagDoll.cpp
//  interface/src/avatar
//
//  Created by Andrew Meadows 2014.05.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include <CollisionInfo.h>
#include <SharedUtil.h>

#include "RagDoll.h"

// ----------------------------------------------------------------------------
// FixedConstraint
// ----------------------------------------------------------------------------
FixedConstraint::FixedConstraint() : _point(NULL), _anchor(0.0f, 0.0f, 0.0f) {
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
DistanceConstraint::DistanceConstraint(glm::vec3* pointA, glm::vec3* pointB) : _distance(-1.0f) {
    _points[0] = pointA;
    _points[1] = pointB;
    _distance = glm::distance(*(_points[0]), *(_points[1]));
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
    float newDistance = glm::distance(*(_points[0]), *(_points[1]));
    glm::vec3 direction(0.0f, 1.0f, 0.0f);
    if (newDistance > EPSILON) {
        direction = (*(_points[0]) - *(_points[1])) / newDistance;
    }
    glm::vec3 center = 0.5f * (*(_points[0]) + *(_points[1]));
    *(_points[0]) = center + (0.5f * _distance) * direction;
    *(_points[1]) = center - (0.5f * _distance) * direction;
    return glm::abs(newDistance - _distance);
}

// ----------------------------------------------------------------------------
// RagDoll
// ----------------------------------------------------------------------------

RagDoll::RagDoll() {
}

RagDoll::~RagDoll() {
    clear();
}
    
void RagDoll::init(const QVector<JointState>& states) {
    clear();
    const int numStates = states.size();
    _points.reserve(numStates);
    for (int i = 0; i < numStates; ++i) {
        const JointState& state = states[i];
        _points.push_back(state.getPosition());
        int parentIndex = state.getFBXJoint().parentIndex;
        assert(parentIndex < i);
        if (parentIndex != -1) {
            DistanceConstraint* stick = new DistanceConstraint(&(_points[i]), &(_points[parentIndex]));
            _constraints.push_back(stick);
        }
    }
}
/// Delete all data.
void RagDoll::clear() {
    int numConstraints = _constraints.size();
    for (int i = 0; i < numConstraints; ++i) {
        delete _constraints[i];
    }
    _constraints.clear();
    _points.clear();
}

float RagDoll::slaveToSkeleton(const QVector<JointState>& states, float fraction) {
    const int numStates = states.size();
    assert(numStates == _points.size());
    fraction = glm::clamp(fraction, 0.0f, 1.0f);
    float maxDistance = 0.0f;
    for (int i = 0; i < numStates; ++i) {
        glm::vec3 oldPoint = _points[i];
        _points[i] = (1.0f - fraction) * _points[i] + fraction * states[i].getPosition();
        maxDistance = glm::max(maxDistance, glm::distance(oldPoint, _points[i]));
    }
    return maxDistance;
}

float RagDoll::enforceConstraints() {
    float maxDistance = 0.0f;
    const int numConstraints = _constraints.size();
    for (int i = 0; i < numConstraints; ++i) {
        DistanceConstraint* c = static_cast<DistanceConstraint*>(_constraints[i]);
        //maxDistance = glm::max(maxDistance, _constraints[i]->enforce());
        maxDistance = glm::max(maxDistance, c->enforce());
    }
    return maxDistance;
}
