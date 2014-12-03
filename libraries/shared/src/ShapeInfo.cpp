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

void ShapeInfo::clear() {
    _type = INVALID_SHAPE;
    _data.clear();
}

void ShapeInfo::setBox(const glm::vec3& halfExtents) {
    _type = BOX_SHAPE;
    _data.clear();
    _data.push_back(halfExtents);
}

void ShapeInfo::setSphere(float radius) {
    _type = SPHERE_SHAPE;
    _data.clear();
    _data.push_back(glm::vec3(radius));
}

void ShapeInfo::setCylinder(float radius, float halfHeight) {
    _type = CYLINDER_SHAPE;
    _data.clear();
    // NOTE: default cylinder has (UpAxis = 1) axis along yAxis and radius stored in X
    _data.push_back(glm::vec3(radius, halfHeight, radius));
}

void ShapeInfo::setCapsule(float radius, float halfHeight) {
    _type = CAPSULE_SHAPE;
    _data.clear();
    _data.push_back(glm::vec3(radius, halfHeight, radius));
}

glm::vec3 ShapeInfo::getBoundingBoxDiagonal() const {
    switch(_type) {
        case BOX_SHAPE:
        case SPHERE_SHAPE:
        case CYLINDER_SHAPE:
        case CAPSULE_SHAPE:
            return 2.0f * _data[0];
        default:
            break;
    }
    return glm::vec3(0.0f);
}
