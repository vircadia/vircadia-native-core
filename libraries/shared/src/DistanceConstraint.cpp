//
//  DistanceConstraint.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.07.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DistanceConstraint.h"
#include "SharedUtil.h" // for EPSILON
#include "VerletPoint.h"

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
