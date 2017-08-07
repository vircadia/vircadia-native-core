//
//  ShapeInfo.cpp
//  libraries/physics/src
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

// Bullet doesn't support arbitrarily small shapes
const float MIN_HALF_EXTENT = 0.005f; // 0.5 cm

void ShapeInfo::clear() {
    _url.clear();
    _pointCollection.clear();
    _triangleIndices.clear();
    _halfExtents = glm::vec3(0.0f);
    _offset = glm::vec3(0.0f);
    _doubleHashKey.clear();
    _type = SHAPE_TYPE_NONE;
}

void ShapeInfo::setParams(ShapeType type, const glm::vec3& halfExtents, QString url) {
    //TODO WL21389: Does this need additional cases and handling added?
    _url = "";
    _type = type;
    setHalfExtents(halfExtents);
    switch(type) {
        case SHAPE_TYPE_NONE:
            _halfExtents = glm::vec3(0.0f);
            break;
        case SHAPE_TYPE_BOX:
            break;
        case SHAPE_TYPE_SPHERE: {
                float radius = glm::length(halfExtents) / SQUARE_ROOT_OF_3;
                radius = glm::max(radius, MIN_HALF_EXTENT);
                _halfExtents = glm::vec3(radius);
            }
            break;
        case SHAPE_TYPE_COMPOUND:
        case SHAPE_TYPE_STATIC_MESH:
            _url = QUrl(url);
            break;
        default:
            break;
    }
    _doubleHashKey.clear();
}

void ShapeInfo::setBox(const glm::vec3& halfExtents) {
    //TODO WL21389:  Should this pointlist clearance added in case
    //              this is a re-purposed instance?
    // See https://github.com/highfidelity/hifi/pull/11024#discussion_r128885491
    _url = "";
    _type = SHAPE_TYPE_BOX;
    setHalfExtents(halfExtents);
    _doubleHashKey.clear();
}

void ShapeInfo::setSphere(float radius) {
    //TODO WL21389: See comment in setBox regarding clearance
    _url = "";
    _type = SHAPE_TYPE_SPHERE;
    radius = glm::max(radius, MIN_HALF_EXTENT);
    _halfExtents = glm::vec3(radius);
    _doubleHashKey.clear();
}

void ShapeInfo::setPointCollection(const ShapeInfo::PointCollection& pointCollection) {
    //TODO WL21389: May need to skip resetting type here.
    _pointCollection = pointCollection;
    _type = (_pointCollection.size() > 0) ? SHAPE_TYPE_COMPOUND : SHAPE_TYPE_NONE;
    _doubleHashKey.clear();
}

void ShapeInfo::setCapsuleY(float radius, float halfHeight) {
    //TODO WL21389: See comment in setBox regarding clearance
    _url = "";
    _type = SHAPE_TYPE_CAPSULE_Y;
    radius = glm::max(radius, MIN_HALF_EXTENT);
    halfHeight = glm::max(halfHeight, 0.0f);
    _halfExtents = glm::vec3(radius, halfHeight, radius);
    _doubleHashKey.clear();
}

void ShapeInfo::setOffset(const glm::vec3& offset) {
    _offset = offset;
    _doubleHashKey.clear();
}

uint32_t ShapeInfo::getNumSubShapes() const {
    switch (_type) {
        case SHAPE_TYPE_NONE:
            return 0;
        case SHAPE_TYPE_COMPOUND:
        case SHAPE_TYPE_SIMPLE_COMPOUND:
            return _pointCollection.size();
        case SHAPE_TYPE_SIMPLE_HULL:
        case SHAPE_TYPE_STATIC_MESH:
            assert(_pointCollection.size() == 1);
            // yes fall through to default
        default:
            return 1;
    }
}

int ShapeInfo::getLargestSubshapePointCount() const {
    int numPoints = 0;
    for (int i = 0; i < _pointCollection.size(); ++i) {
        int n = _pointCollection[i].size();
        if (n > numPoints) {
            numPoints = n;
        }
    }
    return numPoints;
}

float ShapeInfo::computeVolume() const {
    //TODO WL21389: Add support for other ShapeTypes( CYLINDER_X, CYLINDER_Y, etc).
    const float DEFAULT_VOLUME = 1.0f;
    float volume = DEFAULT_VOLUME;
    switch(_type) {
        case SHAPE_TYPE_BOX: {
            // factor of 8.0 because the components of _halfExtents are all halfExtents
            volume = 8.0f * _halfExtents.x * _halfExtents.y * _halfExtents.z;
            break;
        }
        case SHAPE_TYPE_SPHERE: {
            volume = 4.0f * PI * _halfExtents.x * _halfExtents.y * _halfExtents.z / 3.0f;
            break;
        }
        case SHAPE_TYPE_CYLINDER_Y: {
            float radius = _halfExtents.x;
            volume = PI * radius * radius * 2.0f * _halfExtents.y;
            break;
        }
        case SHAPE_TYPE_CAPSULE_Y: {
            float radius = _halfExtents.x;
            // Need to offset halfExtents.y by x to account for the system treating
            // the y extent of the capsule as the cylindrical height + spherical radius.
            float cylinderHeight = 2.0f * (_halfExtents.y - _halfExtents.x);
            volume = PI * radius * radius * (cylinderHeight + 4.0f * radius / 3.0f);
            break;
        }
        default:
            break;
    }
    assert(volume > 0.0f);
    return volume;
}

bool ShapeInfo::contains(const glm::vec3& point) const {
    //TODO WL21389:  Add support for other ShapeTypes like Ellipsoid/Compound.
    switch(_type) {
        case SHAPE_TYPE_SPHERE:
            return glm::length(point) <= _halfExtents.x;
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
    //TODO WL21389: Need to include the pointlist for SIMPLE_HULL in hash
    // NOTE: we cache the key so we only ever need to compute it once for any valid ShapeInfo instance.
    if (_doubleHashKey.isNull() && _type != SHAPE_TYPE_NONE) {
        bool useOffset = glm::length2(_offset) > MIN_SHAPE_OFFSET * MIN_SHAPE_OFFSET;
        // The key is not yet cached therefore we must compute it.

        // compute hash1
        // TODO?: provide lookup table for hash/hash2 of _type rather than recompute?
        uint32_t primeIndex = 0;
        _doubleHashKey.computeHash((uint32_t)_type, primeIndex++);

        // compute hash1
        uint32_t hash = _doubleHashKey.getHash();
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
        _doubleHashKey.setHash(hash);

        // compute hash2
        hash = _doubleHashKey.getHash2();
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
        _doubleHashKey.setHash2(hash);

        if (_type == SHAPE_TYPE_COMPOUND || _type == SHAPE_TYPE_STATIC_MESH) {
            QString url = _url.toString();
            if (!url.isEmpty()) {
                // fold the urlHash into both parts
                QByteArray baUrl = url.toLocal8Bit();
                const char *cUrl = baUrl.data();
                uint32_t urlHash = qChecksum(cUrl, baUrl.count());
                _doubleHashKey.setHash(_doubleHashKey.getHash() ^ urlHash);
                _doubleHashKey.setHash2(_doubleHashKey.getHash2() ^ urlHash);
            }
        }
    }
    return _doubleHashKey;
}

void ShapeInfo::setHalfExtents(const glm::vec3& halfExtents) {
    _halfExtents = glm::max(halfExtents, glm::vec3(MIN_HALF_EXTENT));
}
