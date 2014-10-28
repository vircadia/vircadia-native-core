//
//  VerletSphereShape.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows on 2014.06.16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VerletSphereShape.h"

#include "Ragdoll.h"    // for VerletPoint

VerletSphereShape::VerletSphereShape(VerletPoint* centerPoint) : SphereShape() { 
    assert(centerPoint);
    _point = centerPoint;
}

VerletSphereShape::VerletSphereShape(float radius, VerletPoint* centerPoint) : SphereShape(radius) {
    assert(centerPoint);
    _point = centerPoint;
}

// virtual from Shape class
void VerletSphereShape::setTranslation(const glm::vec3& position) {
    _point->_position = position;
    _point->_lastPosition = position;
}

// virtual from Shape class
const glm::vec3& VerletSphereShape::getTranslation() const {
    return _point->_position;
}

// virtual
float VerletSphereShape::computeEffectiveMass(const glm::vec3& penetration, const glm::vec3& contactPoint) {
    return _point->getMass();
}

// virtual
void VerletSphereShape::accumulateDelta(float relativeMassFactor, const glm::vec3& penetration) {
    _point->accumulateDelta(relativeMassFactor * penetration);
}

// virtual
void VerletSphereShape::applyAccumulatedDelta() {
    _point->applyAccumulatedDelta();
}

void VerletSphereShape::getVerletPoints(QVector<VerletPoint*>& points) {
    points.push_back(_point);
}
