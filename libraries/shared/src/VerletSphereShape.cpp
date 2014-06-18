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

VerletSphereShape::VerletSphereShape(glm::vec3* centerPoint) : SphereShape() { 
    assert(centerPoint);
    _point = centerPoint;
}

VerletSphereShape::VerletSphereShape(float radius, glm::vec3* centerPoint) : SphereShape(radius) {
    assert(centerPoint);
    _point = centerPoint;
}

// virtual from Shape class
void VerletSphereShape::setTranslation(const glm::vec3& position) {
    *_point = position;
}

// virtual from Shape class
glm::vec3 VerletSphereShape::getTranslation() {
    return *_point;
}
