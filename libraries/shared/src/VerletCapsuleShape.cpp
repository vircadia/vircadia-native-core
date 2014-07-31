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

#include "Ragdoll.h"    // for VerletPoint
#include "SharedUtil.h"

VerletCapsuleShape::VerletCapsuleShape(VerletPoint* startPoint, VerletPoint* endPoint) : 
        CapsuleShape(), _startPoint(startPoint), _endPoint(endPoint), _startLagrangeCoef(0.5f), _endLagrangeCoef(0.5f) {
    assert(startPoint);
    assert(endPoint);
    _halfHeight = 0.5f * glm::distance(_startPoint->_position, _endPoint->_position);
    updateBoundingRadius();
}

VerletCapsuleShape::VerletCapsuleShape(float radius, VerletPoint* startPoint, VerletPoint* endPoint) :
        CapsuleShape(radius, 1.0f), _startPoint(startPoint), _endPoint(endPoint), 
        _startLagrangeCoef(0.5f), _endLagrangeCoef(0.5f) {
    assert(startPoint);
    assert(endPoint);
    _halfHeight = 0.5f * glm::distance(_startPoint->_position, _endPoint->_position);
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
    _startPoint->_position = center - halfHeight * axis;
    _endPoint->_position = center + halfHeight * axis;
}

void VerletCapsuleShape::setTranslation(const glm::vec3& position) {
    // NOTE: this method will update the verlet points, which is probably not 
    // what you want to do.  Only call this method if you know what you're doing.

    // update the points such that their center is at position
    glm::vec3 movement = position - getTranslation();
    _startPoint->_position += movement;
    _endPoint->_position += movement;
}

const glm::vec3& VerletCapsuleShape::getTranslation() const {
    // the "translation" of this shape must be computed on the fly
    VerletCapsuleShape* thisCapsule = const_cast<VerletCapsuleShape*>(this);
    thisCapsule->_translation = 0.5f * (_startPoint->_position + _endPoint->_position);
    return _translation;
}

float VerletCapsuleShape::computeEffectiveMass(const glm::vec3& penetration, const glm::vec3& contactPoint) {
    glm::vec3 startLeg = _startPoint->_position - contactPoint;
    glm::vec3 endLeg = _endPoint->_position - contactPoint;

    // TODO: use fast approximate distance calculations here
    float startLength = glm::length(startLeg);
    float endlength = glm::length(endLeg);

    // The raw coefficient is proportional to the other leg's length multiplied by the dot-product
    // of the penetration and this leg direction.  We don't worry about the common penetration length 
    // because it is normalized out later.
    float startCoef = glm::abs(glm::dot(startLeg, penetration)) * endlength / (startLength + EPSILON);
    float endCoef = glm::abs(glm::dot(endLeg, penetration)) * startLength / (endlength + EPSILON);

    float maxCoef = glm::max(startCoef, endCoef);
    if (maxCoef > EPSILON) {
        // One of these coeficients will be 1.0, the other will be less --> 
        // one endpoint will move the full amount while the other will move less.
        _startLagrangeCoef = startCoef / maxCoef;
        _endLagrangeCoef = endCoef / maxCoef;
    } else {
        // The coefficients are the same --> the collision will move both equally
        // as if the contact were at the center of mass.
        _startLagrangeCoef = 1.0f;
        _endLagrangeCoef = 1.0f;
    }
    // the effective mass is the weighted sum of the two endpoints
    return _startLagrangeCoef * _startPoint->_mass + _endLagrangeCoef * _endPoint->_mass;
}

void VerletCapsuleShape::accumulateDelta(float relativeMassFactor, const glm::vec3& penetration) {
    assert(!glm::isnan(relativeMassFactor));
    _startPoint->accumulateDelta((relativeMassFactor * _startLagrangeCoef) * penetration);
    _endPoint->accumulateDelta((relativeMassFactor * _endLagrangeCoef) * penetration);
}

void VerletCapsuleShape::applyAccumulatedDelta() {
    _startPoint->applyAccumulatedDelta();
    _endPoint->applyAccumulatedDelta();
}

// virtual
float VerletCapsuleShape::getHalfHeight() const {
    return 0.5f * glm::distance(_startPoint->_position, _endPoint->_position);
}

// virtual
void VerletCapsuleShape::getStartPoint(glm::vec3& startPoint) const {
    startPoint = _startPoint->_position;
}

// virtual
void VerletCapsuleShape::getEndPoint(glm::vec3& endPoint) const {
    endPoint = _endPoint->_position;
}

// virtual
void VerletCapsuleShape::computeNormalizedAxis(glm::vec3& axis) const {
    glm::vec3 unormalizedAxis = _endPoint->_position - _startPoint->_position;
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
    _startPoint->_position = center - halfHeight * axis;
    _endPoint->_position = center + halfHeight * axis;
    _boundingRadius = _radius + halfHeight;
}

// virtual 
void VerletCapsuleShape::setRadiusAndHalfHeight(float radius, float halfHeight) {
    _radius = radius;
    setHalfHeight(halfHeight);
}

// virtual
void VerletCapsuleShape::setEndPoints(const glm::vec3& startPoint, const glm::vec3& endPoint) {
    _startPoint->_position = startPoint;
    _endPoint->_position = endPoint;
    updateBoundingRadius();
}
