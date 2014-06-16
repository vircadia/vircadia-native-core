//
//  VerletCapsuleShape.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows on 2014.06.16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VerletCapsuleShape.h"
#include "SharedUtil.h"

VerletCapsuleShape::VerletCapsuleShape(glm::vec3* startPoint, glm::vec3* endPoint) : 
        CapsuleShape(), _startPoint(startPoint), _endPoint(endPoint) {
    assert(startPoint);
    assert(endPoint);
    _halfHeight = 0.5f * glm::distance(*_startPoint, *_endPoint);
    updateBoundingRadius();
}

VerletCapsuleShape::VerletCapsuleShape(float radius, glm::vec3* startPoint, glm::vec3* endPoint) :
        CapsuleShape(radius, 1.0f), _startPoint(startPoint), _endPoint(endPoint) {
    assert(startPoint);
    assert(endPoint);
    _halfHeight = 0.5f * glm::distance(*_startPoint, *_endPoint);
    updateBoundingRadius();
}

// virtual
float VerletCapsuleShape::getHalfHeight() const {
    return 0.5f * glm::distance(*_startPoint, *_endPoint);
}

// virtual
void VerletCapsuleShape::getStartPoint(glm::vec3& startPoint) const {
    startPoint = *_startPoint;
}

// virtual
void VerletCapsuleShape::getEndPoint(glm::vec3& endPoint) const {
    endPoint = *_endPoint;
}

// virtual
void VerletCapsuleShape::computeNormalizedAxis(glm::vec3& axis) const {
    glm::vec3 unormalizedAxis = *_endPoint - *_startPoint;
    float fullLength = glm::length(unormalizedAxis);
    if (fullLength > EPSILON) {
        axis = unormalizedAxis / fullLength;
    } else {
        // the axis is meaningless, but we fill it with a normalized direction
        // just in case the calling context assumes it really is normalized.
        axis = glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

// virtual 
void VerletCapsuleShape::setHalfHeight(float height) {
    // don't call this method because this is a verlet shape
    assert(false);
}

// virtual 
void VerletCapsuleShape::setRadiusAndHalfHeight(float radius, float height) {
    // don't call this method because this is a verlet shape
    assert(false);
}

// virtual
void VerletCapsuleShape::setEndPoints(const glm::vec3& startPoint, const glm::vec3& endPoint) {
    *_startPoint = startPoint;
    *_endPoint = endPoint;
    updateBoundingRadius();
}
