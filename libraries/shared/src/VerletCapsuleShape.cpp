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

const glm::quat& VerletCapsuleShape::getRotation() const {
    // NOTE: The "rotation" of this shape must be computed on the fly, 
    // which makes this method MUCH more more expensive than you might expect.
    glm::vec3 axis;
    computeNormalizedAxis(axis);
    VerletCapsuleShape* thisCapsule = const_cast<VerletCapsuleShape*>(this);
    thisCapsule->_rotation = computeNewRotation(axis);
    return _rotation;
}

void VerletCapsuleShape::setRotation(const glm::quat& rotation) {
    // NOTE: this method will update the verlet points, which is probably not 
    // what you want to do.  Only call this method if you know what you're doing.

    // update points such that they have the same center but a different axis
    glm::vec3 center = getTranslation();
    float halfHeight = getHalfHeight();
    glm::vec3 axis = rotation * DEFAULT_CAPSULE_AXIS;
    *_startPoint = center - halfHeight * axis;
    *_endPoint = center + halfHeight * axis;
}

void VerletCapsuleShape::setTranslation(const glm::vec3& position) {
    // NOTE: this method will update the verlet points, which is probably not 
    // what you want to do.  Only call this method if you know what you're doing.

    // update the points such that their center is at position
    glm::vec3 movement = position - getTranslation();
    *_startPoint += movement;
    *_endPoint += movement;
}

const glm::vec3& VerletCapsuleShape::getTranslation() const {
    // the "translation" of this shape must be computed on the fly
    VerletCapsuleShape* thisCapsule = const_cast<VerletCapsuleShape*>(this);
    thisCapsule->_translation = 0.5f * ((*_startPoint) + (*_endPoint));
    return _translation;
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
void VerletCapsuleShape::setHalfHeight(float halfHeight) {
    // push points along axis so they are 2*halfHeight apart
    glm::vec3 center = getTranslation();
    glm::vec3 axis;
    computeNormalizedAxis(axis);
    *_startPoint = center - halfHeight * axis;
    *_endPoint = center + halfHeight * axis;
    _boundingRadius = _radius + halfHeight;
}

// virtual 
void VerletCapsuleShape::setRadiusAndHalfHeight(float radius, float halfHeight) {
    _radius = radius;
    setHalfHeight(halfHeight);
}

// virtual
void VerletCapsuleShape::setEndPoints(const glm::vec3& startPoint, const glm::vec3& endPoint) {
    *_startPoint = startPoint;
    *_endPoint = endPoint;
    updateBoundingRadius();
}
