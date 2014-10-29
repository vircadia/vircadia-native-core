//
//  ContactConstraint.cpp
//  interface/src/avatar
//
//  Created by Andrew Meadows 2014.07.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <SharedUtil.h>

#include "ContactConstraint.h"
#include "VerletPoint.h"


ContactConstraint::ContactConstraint(VerletPoint* pointA, VerletPoint* pointB) 
        : _pointA(pointA), _pointB(pointB), _strength(1.0f) {
    assert(_pointA != NULL && _pointB != NULL);
    _offset = _pointB->_position - _pointA->_position;
}

float ContactConstraint::enforce() {
    _pointB->_position += _strength * (_pointA->_position + _offset - _pointB->_position);
    return 0.0f;
}

float ContactConstraint::enforceWithNormal(const glm::vec3& normal) {
    glm::vec3 delta = _pointA->_position + _offset - _pointB->_position;

    // split delta into parallel (pDelta) and perpendicular (qDelta) components
    glm::vec3 pDelta = glm::dot(delta, normal) * normal;
    glm::vec3 qDelta = delta - pDelta;

    // use the relative sizes of the components to decide how much perpenducular delta to use
    // (i.e. dynamic friction)
    float lpDelta = glm::length(pDelta);
    float lqDelta = glm::length(qDelta);
    float qFactor = lqDelta > lpDelta ? (lpDelta / lqDelta - 1.0f) : 0.0f;
    // recombine the two components to get the final delta
    delta = pDelta + qFactor * qDelta;

    // attenuate strength by how much _offset is perpendicular to normal
    float distance = glm::length(_offset);
    float strength = _strength * ((distance > EPSILON) ? glm::abs(glm::dot(_offset, normal)) / distance : 1.0f);

    // move _pointB
    _pointB->_position += strength * delta;

    return strength * glm::length(delta);
}

