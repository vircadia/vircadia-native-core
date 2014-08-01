//
//  ContactConstraint.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.07.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ContactConstraint.h"
#include "Shape.h"
#include "SharedUtil.h"

ContactConstraint::ContactConstraint() : _lastFrame(0), _shapeA(NULL), _shapeB(NULL), 
        _offsetA(0.0f), _offsetB(0.0f), _normal(0.0f) {
}

ContactConstraint::ContactConstraint(const CollisionInfo& collision, quint32 frame) : _lastFrame(frame),
        _shapeA(collision.getShapeA()), _shapeB(collision.getShapeB()), _offsetA(0.0f), _offsetB(0.0f), _normal(0.0f) {

    _offsetA = collision._contactPoint - _shapeA->getTranslation();
    _offsetB = collision._contactPoint - collision._penetration - _shapeB->getTranslation();
    float pLength = glm::length(collision._penetration);
    if (pLength > EPSILON) {
        _normal = collision._penetration / pLength;
    }

    if (_shapeA->getID() > _shapeB->getID()) {
        // swap so that _shapeA always has lower ID
        _shapeA = collision.getShapeB();
        _shapeB = collision.getShapeA();

        glm::vec3 temp = _offsetA;
        _offsetA = _offsetB;
        _offsetB = temp;
        _normal = - _normal;
    }
}

// virtual 
float ContactConstraint::enforce() {
    glm::vec3 pointA = _shapeA->getTranslation() + _offsetA;
    glm::vec3 pointB = _shapeB->getTranslation() + _offsetB;
    glm::vec3 penetration = pointA - pointB;
    float pDotN = glm::dot(penetration, _normal);
    if (pDotN > EPSILON) {
        penetration = (0.99f * pDotN) * _normal;
        // NOTE: Shape::computeEffectiveMass() has side effects: computes and caches partial Lagrangian coefficients
        // which are then used in the accumulateDelta() calls below.
        float massA = _shapeA->computeEffectiveMass(penetration, pointA);
        float massB = _shapeB->computeEffectiveMass(-penetration, pointB);
        float totalMass = massA + massB;
        if (totalMass < EPSILON) {
            massA = massB = 1.0f;
            totalMass = 2.0f;
        }
        // NOTE: Shape::accumulateDelta() uses the coefficients from previous call to Shape::computeEffectiveMass()
        // and remember that penetration points from A into B
        _shapeA->accumulateDelta(massB / totalMass, -penetration);
        _shapeB->accumulateDelta(massA / totalMass, penetration);
        return pDotN;
    }
    return 0.0f;
}

void ContactConstraint::updateContact(const CollisionInfo& collision, quint32 frame) {
    _lastFrame = frame;
    _offsetA = collision._contactPoint - collision._shapeA->getTranslation();
    _offsetB = collision._contactPoint - collision._penetration - collision._shapeB->getTranslation();
    float pLength = glm::length(collision._penetration);
    if (pLength > EPSILON) {
        _normal = collision._penetration / pLength;
    } else {
        _normal = glm::vec3(0.0f);
    }
    if (collision._shapeA->getID() > collision._shapeB->getID()) {
        // our _shapeA always has lower ID
        glm::vec3 temp = _offsetA;
        _offsetA = _offsetB;
        _offsetB = temp;
        _normal = - _normal;
    }
}
