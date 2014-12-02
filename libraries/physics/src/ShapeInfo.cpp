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

#ifdef USE_BULLET_PHYSICS

#include <math.h>

#include <SharedUtil.h> // for MILLIMETERS_PER_METER

#include "BulletUtil.h"
#include "DoubleHashKey.h"
#include "ShapeInfo.h"

void ShapeInfo::setBox(const glm::vec3& halfExtents) {
    _type = BOX_SHAPE_PROXYTYPE;
    _data.clear();
    _data.push_back(halfExtents);
}

void ShapeInfo::setBox(const glm::vec3& halfExtents) {
    _type = BOX_SHAPE_PROXYTYPE;
    _data.clear();
    _data.push_back(bulletHalfExtents);
}

void ShapeInfo::setSphere(float radius) {
    _type = SPHERE_SHAPE_PROXYTYPE;
    _data.clear();
    _data.push_back(glm::vec3(0.0f, 0.0f, radius));
}

void ShapeInfo::setCylinder(float radius, float height) {
    _type = CYLINDER_SHAPE_PROXYTYPE;
    _data.clear();
    // NOTE: default cylinder has (UpAxis = 1) axis along yAxis and radius stored in X
    _data.push_back(glm::vec3btVector3(radius, 0.5f * height, radius));
}

void ShapeInfo::setCapsule(float radius, float height) {
    _type = CAPSULE_SHAPE_PROXYTYPE;
    _data.clear();
    _data.push_back(glm::vec3(radius, 0.5f * height, 0.0f));
}

int ShapeInfo::computeHash() const {
    // scramble the bits of the type
    // TODO?: provide lookup table for hash of _type?
    int primeIndex = 0;
    unsigned int hash = DoubleHashKey::hashFunction((unsigned int)_type, primeIndex++);

    glm::vec3 tmpData;
    int numData = _data.size();
    for (int i = 0; i < numData; ++i) {
        tmpData = _data[i];
        for (int j = 0; j < 3; ++j) {
            // NOTE: 0.49f is used to bump the float up almost half a millimeter
            // so the cast to int produces a round() effect rather than a floor()
            unsigned int floatHash = 
                DoubleHashKey::hashFunction((int)(tmpData[j] * MILLIMETERS_PER_METER + copysignf(1.0f, tmpData[j]) * 0.49f), primeIndex++);
            hash ^= floatHash;
        }
    }
    return hash;
}

int ShapeInfo::computeHash2() const {
    // scramble the bits of the type
    // TODO?: provide lookup table for hash of _type?
    unsigned int hash = DoubleHashKey::hashFunction2((unsigned int)_type);

    glm::vec3 tmpData;
    int numData = _data.size();
    for (int i = 0; i < numData; ++i) {
        tmpData = _data[i];
        for (int j = 0; j < 3; ++j) {
            // NOTE: 0.49f is used to bump the float up almost half a millimeter
            // so the cast to int produces a round() effect rather than a floor()
            unsigned int floatHash = 
                DoubleHashKey::hashFunction2((int)(tmpData[j] * MILLIMETERS_PER_METER + copysignf(1.0f, tmpData[j]) * 0.49f));
            hash += ~(floatHash << 17);
            hash ^=  (floatHash >> 11);
            hash +=  (floatHash << 4); 
            hash ^=  (floatHash >> 7); 
            hash += ~(floatHash << 10);
            hash = (hash << 16) | (hash >> 16);
        }
    }
    return hash;
}

#endif // USE_BULLET_PHYSICS
