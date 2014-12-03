//
//  ShapeInfo.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <math.h>

#include "SharedUtil.h" // for MILLIMETERS_PER_METER

//#include "DoubleHashKey.h"
#include "ShapeInfo.h"

void ShapeInfo::setBox(const glm::vec3& halfExtents) {
    _type = BOX_SHAPE;
    _data.clear();
    _data.push_back(halfExtents);
}

void ShapeInfo::setSphere(float radius) {
    _type = SPHERE_SHAPE;
    _data.clear();
    _data.push_back(glm::vec3(0.0f, 0.0f, radius));
}

void ShapeInfo::setCylinder(float radius, float height) {
    _type = CYLINDER_SHAPE;
    _data.clear();
    // NOTE: default cylinder has (UpAxis = 1) axis along yAxis and radius stored in X
    _data.push_back(glm::vec3(radius, 0.5f * height, radius));
}

void ShapeInfo::setCapsule(float radius, float height) {
    _type = CAPSULE_SHAPE;
    _data.clear();
    _data.push_back(glm::vec3(radius, 0.5f * height, 0.0f));
}

