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
    // _data[0] = < halfX, halfY, halfZ >
    _data.push_back(halfExtents);
}

void ShapeInfo::setSphere(float radius) {
    _type = SPHERE_SHAPE;
    _data.clear();
    // _data[0] = < radius, radius, radius >
    _data.push_back(glm::vec3(radius));
}

void ShapeInfo::setCylinder(float radius, float halfHeight) {
    _type = CYLINDER_SHAPE;
    _data.clear();
    // _data[0] = < radius, halfHeight, radius >
    // NOTE: default cylinder has (UpAxis = 1) axis along yAxis and radius stored in X
    _data.push_back(glm::vec3(radius, halfHeight, radius));
}

void ShapeInfo::setCapsule(float radius, float halfHeight) {
    _type = CAPSULE_SHAPE;
    _data.clear();
    // _data[0] = < radius, halfHeight, radius >
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

float ShapeInfo::computeVolume() const {
    const float DEFAULT_VOLUME = 1.0f;
    float volume = DEFAULT_VOLUME;
    switch(_type) {
        case BOX_SHAPE: {
            // factor of 8.0 because the components of _data[0] are all halfExtents
            volume = 8.0f * _data[0].x * _data[0].y * _data[0].z;
            break;
        }
        case SPHERE_SHAPE: {
            float radius = _data[0].x;
            volume = 4.0f * PI * radius * radius * radius / 3.0f;
            break;
        }
        case CYLINDER_SHAPE: {
            float radius = _data[0].x;
            volume = PI * radius * radius * 2.0f * _data[0].y;
            break;
        }
        case CAPSULE_SHAPE: {
            float radius = _data[0].x;
            volume = PI * radius * radius * (2.0f * _data[0].y + 4.0f * radius / 3.0f);
            break;
        }
        default:
            break;
    }
    assert(volume > 0.0f);
    return volume;
}
