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

#include "ShapeInfo.h"

#include <math.h>

#include "NumericalConstants.h" // for MILLIMETERS_PER_METER

void ShapeInfo::clear() {
    _type = SHAPE_TYPE_NONE;
    _halfExtents = _offset = glm::vec3(0.0f);
    _doubleHashKey.clear();
}

void ShapeInfo::setParams(ShapeType type, const glm::vec3& halfExtents, QString url) {
    _type = type;
    _points.clear();
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
        case SHAPE_TYPE_COMPOUND:
            _url = QUrl(url);
            _halfExtents = halfExtents;
            break;
        default:
            _halfExtents = halfExtents;
            break;
    }
    _doubleHashKey.clear();
}

void ShapeInfo::setBox(const glm::vec3& halfExtents) {
    _url = "";
    _type = SHAPE_TYPE_BOX;
    _halfExtents = halfExtents;
    _points.clear();
    _doubleHashKey.clear();
}

void ShapeInfo::setSphere(float radius) {
    _url = "";
    _type = SHAPE_TYPE_SPHERE;
    _halfExtents = glm::vec3(radius, radius, radius);
    _points.clear();
    _doubleHashKey.clear();
}

void ShapeInfo::setEllipsoid(const glm::vec3& halfExtents) {
    _url = "";
    _type = SHAPE_TYPE_ELLIPSOID;
    _halfExtents = halfExtents;
    _points.clear();
    _doubleHashKey.clear();
}

void ShapeInfo::setConvexHulls(const QVector<QVector<glm::vec3>>& points) {
    _points = points;
    _type = (_points.size() > 0) ? SHAPE_TYPE_COMPOUND : SHAPE_TYPE_NONE;
    _doubleHashKey.clear();
}

void ShapeInfo::setCapsuleY(float radius, float halfHeight) {
    _url = "";
    _type = SHAPE_TYPE_CAPSULE_Y;
    _halfExtents = glm::vec3(radius, halfHeight, radius);
    _points.clear();
    _doubleHashKey.clear();
}

void ShapeInfo::setOffset(const glm::vec3& offset) {
    _offset = offset;
    _doubleHashKey.clear();
}

uint32_t ShapeInfo::getNumSubShapes() const {
    if (_type == SHAPE_TYPE_NONE) {
        return 0;
    } else if (_type == SHAPE_TYPE_COMPOUND) {
        return _points.size();
    }
    return 1;
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
            volume = PI * radius * radius * (2.0f * (_halfExtents.y - _halfExtents.x) + 4.0f * radius / 3.0f);
            break;
        }
        default:
            break;
    }
    assert(volume > 0.0f);
    return volume;
}

bool ShapeInfo::contains(const glm::vec3& point) const {
    switch(_type) {
        case SHAPE_TYPE_SPHERE:
            return glm::length(point) <= _halfExtents.x;
        case SHAPE_TYPE_ELLIPSOID: {
            glm::vec3 scaledPoint = glm::abs(point) / _halfExtents;
            return glm::length(scaledPoint) <= 1.0f;
        }
        case SHAPE_TYPE_CYLINDER_X:
            return glm::length(glm::vec2(point.y, point.z)) <= _halfExtents.z;
        case SHAPE_TYPE_CYLINDER_Y:
            return glm::length(glm::vec2(point.x, point.z)) <= _halfExtents.x;
        case SHAPE_TYPE_CYLINDER_Z:
            return glm::length(glm::vec2(point.x, point.y)) <= _halfExtents.y;
        case SHAPE_TYPE_CAPSULE_X: {
            if (glm::abs(point.x) <= _halfExtents.x) {
                return glm::length(glm::vec2(point.y, point.z)) <= _halfExtents.z;
            } else {
                glm::vec3 absPoint = glm::abs(point) - _halfExtents.x;
                return glm::length(absPoint) <= _halfExtents.z;
            }
        }
        case SHAPE_TYPE_CAPSULE_Y: {
            if (glm::abs(point.y) <= _halfExtents.y) {
                return glm::length(glm::vec2(point.x, point.z)) <= _halfExtents.x;
            } else {
                glm::vec3 absPoint = glm::abs(point) - _halfExtents.y;
                return glm::length(absPoint) <= _halfExtents.x;
            }
        }
        case SHAPE_TYPE_CAPSULE_Z: {
            if (glm::abs(point.z) <= _halfExtents.z) {
                return glm::length(glm::vec2(point.x, point.y)) <= _halfExtents.y;
            } else {
                glm::vec3 absPoint = glm::abs(point) - _halfExtents.z;
                return glm::length(absPoint) <= _halfExtents.y;
            }
        }
        case SHAPE_TYPE_BOX:
        default: {
            glm::vec3 absPoint = glm::abs(point);
            return absPoint.x <= _halfExtents.x
            && absPoint.y <= _halfExtents.y
            && absPoint.z <= _halfExtents.z;
        }
    }
}

const DoubleHashKey& ShapeInfo::getHash() const {
    // NOTE: we cache the key so we only ever need to compute it once for any valid ShapeInfo instance.
    if (_doubleHashKey.isNull() && _type != SHAPE_TYPE_NONE) {
        bool useOffset = glm::length2(_offset) > MIN_SHAPE_OFFSET * MIN_SHAPE_OFFSET;
        // The key is not yet cached therefore we must compute it!  To this end we bypass the const-ness
        // of this method by grabbing a non-const pointer to "this" and a non-const reference to _doubleHashKey.
        ShapeInfo* thisPtr = const_cast<ShapeInfo*>(this);
        DoubleHashKey& key = thisPtr->_doubleHashKey;

        // compute hash1
        // TODO?: provide lookup table for hash/hash2 of _type rather than recompute?
        uint32_t primeIndex = 0;
        key.computeHash((uint32_t)_type, primeIndex++);
    
        // compute hash1 
        uint32_t hash = key.getHash();
        for (int j = 0; j < 3; ++j) {
            // NOTE: 0.49f is used to bump the float up almost half a millimeter
            // so the cast to int produces a round() effect rather than a floor()
            hash ^= DoubleHashKey::hashFunction(
                    (uint32_t)(_halfExtents[j] * MILLIMETERS_PER_METER + copysignf(1.0f, _halfExtents[j]) * 0.49f), 
                    primeIndex++);
            if (useOffset) {
                hash ^= DoubleHashKey::hashFunction(
                        (uint32_t)(_offset[j] * MILLIMETERS_PER_METER + copysignf(1.0f, _offset[j]) * 0.49f), 
                        primeIndex++);
            }
        }
        key.setHash(hash);
    
        // compute hash2
        hash = key.getHash2();
        for (int j = 0; j < 3; ++j) {
            // NOTE: 0.49f is used to bump the float up almost half a millimeter
            // so the cast to int produces a round() effect rather than a floor()
            uint32_t floatHash = DoubleHashKey::hashFunction2(
                    (uint32_t)(_halfExtents[j] * MILLIMETERS_PER_METER + copysignf(1.0f, _halfExtents[j]) * 0.49f));
            if (useOffset) {
                floatHash ^= DoubleHashKey::hashFunction2(
                        (uint32_t)(_offset[j] * MILLIMETERS_PER_METER + copysignf(1.0f, _offset[j]) * 0.49f));
            }
            hash += ~(floatHash << 17);
            hash ^=  (floatHash >> 11);
            hash +=  (floatHash << 4);
            hash ^=  (floatHash >> 7);
            hash += ~(floatHash << 10);
            hash = (hash << 16) | (hash >> 16);
        }
        key.setHash2(hash);

        QString url = _url.toString();
        if (!url.isEmpty()) {
            // fold the urlHash into both parts
            QByteArray baUrl = url.toLocal8Bit();
            const char *cUrl = baUrl.data();
            uint32_t urlHash = qChecksum(cUrl, baUrl.count());
            key.setHash(key.getHash() ^ urlHash);
            key.setHash2(key.getHash2() ^ urlHash);
        }
    }
    return _doubleHashKey;
}
