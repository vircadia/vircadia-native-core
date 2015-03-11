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

#include "ShapeInfo.h"

void ShapeInfo::clear() {
    _type = SHAPE_TYPE_NONE;
    _halfExtents = glm::vec3(0.0f);
    _doubleHashKey.clear();
    _externalData = NULL;
}

void ShapeInfo::setParams(ShapeType type, const glm::vec3& halfExtents, QVector<glm::vec3>* data, QString url) {
    _type = type;
    switch(type) {
        case SHAPE_TYPE_NONE:
            _halfExtents = glm::vec3(0.0f);
            break;
        case SHAPE_TYPE_BOX:
            _halfExtents = halfExtents;
            break;
        case SHAPE_TYPE_SPHERE: {
            // sphere radius is max of halfExtents
            float radius = glm::max(glm::max(halfExtents.x, halfExtents.y), halfExtents.z);
            _halfExtents = glm::vec3(radius);
            break;
        }
        case SHAPE_TYPE_CONVEX_HULL:
            _url = QUrl(url);
            // start download of model which contains collision hulls
            _type = SHAPE_TYPE_NONE; // until download is done
            break;
        default:
            _halfExtents = halfExtents;
            break;
    }
    _externalData = data;
}


void ShapeInfo::collisionGeometryLoaded() {
    _type = SHAPE_TYPE_CONVEX_HULL;
    // xxx copy points over;
}


void ShapeInfo::setBox(const glm::vec3& halfExtents) {
    _type = SHAPE_TYPE_BOX;
    _halfExtents = halfExtents;
    _doubleHashKey.clear();
}

void ShapeInfo::setSphere(float radius) {
    _type = SHAPE_TYPE_SPHERE;
    _halfExtents = glm::vec3(radius, radius, radius);
    _doubleHashKey.clear();
}

void ShapeInfo::setEllipsoid(const glm::vec3& halfExtents) {
    _type = SHAPE_TYPE_ELLIPSOID;
    _halfExtents = halfExtents;
    _doubleHashKey.clear();
}

void ShapeInfo::setConvexHull(QVector<glm::vec3>& points) {
    _type = SHAPE_TYPE_CONVEX_HULL;
    _points = points;
}

void ShapeInfo::setCapsuleY(float radius, float halfHeight) {
    _type = SHAPE_TYPE_CAPSULE_Y;
    _halfExtents = glm::vec3(radius, halfHeight, radius);
    _doubleHashKey.clear();
}

float ShapeInfo::computeVolume() const {
    const float DEFAULT_VOLUME = 1.0f;
    float volume = DEFAULT_VOLUME;
    switch(_type) {
        case SHAPE_TYPE_BOX: {
            // factor of 8.0 because the components of _halfExtents are all halfExtents
            volume = 8.0f * _halfExtents.x * _halfExtents.y * _halfExtents.z;
            break;
        }
        case SHAPE_TYPE_SPHERE: {
            float radius = _halfExtents.x;
            volume = 4.0f * PI * radius * radius * radius / 3.0f;
            break;
        }
        case SHAPE_TYPE_CYLINDER_Y: {
            float radius = _halfExtents.x;
            volume = PI * radius * radius * 2.0f * _halfExtents.y;
            break;
        }
        case SHAPE_TYPE_CAPSULE_Y: {
            float radius = _halfExtents.x;
            volume = PI * radius * radius * (2.0f * _halfExtents.y + 4.0f * radius / 3.0f);
            break;
        }
        default:
            break;
    }
    assert(volume > 0.0f);
    return volume;
}

const DoubleHashKey& ShapeInfo::getHash() const {
    // NOTE: we cache the hash so we only ever need to compute it once for any valid ShapeInfo instance.
    if (_doubleHashKey.isNull() && _type != SHAPE_TYPE_NONE) {
        // cast this to non-const pointer so we can do our dirty work
        ShapeInfo* thisPtr = const_cast<ShapeInfo*>(this);
        // compute hash1
        // TODO?: provide lookup table for hash/hash2 of _type rather than recompute?
        uint32_t primeIndex = 0;
        thisPtr->_doubleHashKey.computeHash((uint32_t)_type, primeIndex++);
    
        const QVector<glm::vec3>* data = getData();
        if (data) {
            // if externalData exists we use it to continue the hash

            // compute hash
            uint32_t hash = _doubleHashKey.getHash();
    
            glm::vec3 tmpData;
            int numData = data->size();
            for (int i = 0; i < numData; ++i) {
                tmpData = (*data)[i];
                for (int j = 0; j < 3; ++j) {
                    // NOTE: 0.49f is used to bump the float up almost half a millimeter
                    // so the cast to int produces a round() effect rather than a floor()
                    uint32_t floatHash =
                        DoubleHashKey::hashFunction((uint32_t)(tmpData[j] * MILLIMETERS_PER_METER + copysignf(1.0f, tmpData[j]) * 0.49f), primeIndex++);
                    hash ^= floatHash;
                }
            }
            thisPtr->_doubleHashKey.setHash(hash);
        
            // compute hash2
            hash = _doubleHashKey.getHash2();
            for (int i = 0; i < numData; ++i) {
                tmpData = (*data)[i];
                for (int j = 0; j < 3; ++j) {
                    // NOTE: 0.49f is used to bump the float up almost half a millimeter
                    // so the cast to int produces a round() effect rather than a floor()
                    uint32_t floatHash =
                        DoubleHashKey::hashFunction2((uint32_t)(tmpData[j] * MILLIMETERS_PER_METER + copysignf(1.0f, tmpData[j]) * 0.49f));
                    hash += ~(floatHash << 17);
                    hash ^=  (floatHash >> 11);
                    hash +=  (floatHash << 4);
                    hash ^=  (floatHash >> 7);
                    hash += ~(floatHash << 10);
                    hash = (hash << 16) | (hash >> 16);
                }
            }
            thisPtr->_doubleHashKey.setHash2(hash);
        } else {
            // this shape info has no external data so type+extents should be enough to generate a unique hash
            // compute hash1
            uint32_t hash = _doubleHashKey.getHash();
            for (int j = 0; j < 3; ++j) {
                // NOTE: 0.49f is used to bump the float up almost half a millimeter
                // so the cast to int produces a round() effect rather than a floor()
                uint32_t floatHash =
                    DoubleHashKey::hashFunction((uint32_t)(_halfExtents[j] * MILLIMETERS_PER_METER + copysignf(1.0f, _halfExtents[j]) * 0.49f), primeIndex++);
                hash ^= floatHash;
            }
            thisPtr->_doubleHashKey.setHash(hash);
        
            // compute hash2
            hash = _doubleHashKey.getHash2();
            for (int j = 0; j < 3; ++j) {
                // NOTE: 0.49f is used to bump the float up almost half a millimeter
                // so the cast to int produces a round() effect rather than a floor()
                uint32_t floatHash =
                    DoubleHashKey::hashFunction2((uint32_t)(_halfExtents[j] * MILLIMETERS_PER_METER + copysignf(1.0f, _halfExtents[j]) * 0.49f));
                hash += ~(floatHash << 17);
                hash ^=  (floatHash >> 11);
                hash +=  (floatHash << 4);
                hash ^=  (floatHash >> 7);
                hash += ~(floatHash << 10);
                hash = (hash << 16) | (hash >> 16);
            }
            thisPtr->_doubleHashKey.setHash2(hash);
        }
    }
    return _doubleHashKey;
}
