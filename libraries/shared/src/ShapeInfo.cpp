//
//  ShapeInfo.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ShapeInfo.h"

#include <math.h>

#include "HashKey.h"
#include "NumericalConstants.h" // for MILLIMETERS_PER_METER

/*@jsdoc
 * <p>Defines the shape used for collisions or zones.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"none"</code></td><td>No shape.</td></tr>
 *     <tr><td><code>"box"</code></td><td>A cube.</td></tr>
 *     <tr><td><code>"sphere"</code></td><td>A sphere.</td></tr>
 *     <tr><td><code>"capsule-x"</code></td><td>A capsule (cylinder with spherical ends) oriented on the x-axis.</td></tr>
 *     <tr><td><code>"capsule-y"</code></td><td>A capsule (cylinder with spherical ends) oriented on the y-axis.</td></tr>
 *     <tr><td><code>"capsule-z"</code></td><td>A capsule (cylinder with spherical ends) oriented on the z-axis.</td></tr>
 *     <tr><td><code>"cylinder-x"</code></td><td>A cylinder oriented on the x-axis.</td></tr>
 *     <tr><td><code>"cylinder-y"</code></td><td>A cylinder oriented on the y-axis.</td></tr>
 *     <tr><td><code>"cylinder-z"</code></td><td>A cylinder oriented on the z-axis.</td></tr>
 *     <tr><td><code>"hull"</code></td><td><em>Not used.</em></td></tr>
 *     <tr><td><code>"compound"</code></td><td>A compound convex hull specified in an OBJ file.</td></tr>
 *     <tr><td><code>"simple-hull"</code></td><td>A convex hull automatically generated from the model.</td></tr>
 *     <tr><td><code>"simple-compound"</code></td><td>A compound convex hull automatically generated from the model, using 
 *         sub-meshes.</td></tr>
 *     <tr><td><code>"static-mesh"</code></td><td>The exact shape of the model.</td></tr>
 *     <tr><td><code>"plane"</code></td><td>A plane.</td></tr>
 *     <tr><td><code>"ellipsoid"</code></td><td>An ellipsoid.</td></tr>
 *     <tr><td><code>"circle"</code></td><td>A circle.</td></tr>
 *     <tr><td><code>"multisphere"</code></td><td>A convex hull generated from a set of spheres.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} ShapeType
 */
// Originally within EntityItemProperties.cpp
const char* shapeTypeNames[] = {
    "none",
    "box",
    "sphere",
    "capsule-x",
    "capsule-y",
    "capsule-z",
    "cylinder-x",
    "cylinder-y",
    "cylinder-z",
    "hull",
    "plane",
    "compound",
    "simple-hull",
    "simple-compound",
    "static-mesh",
    "ellipsoid",
    "circle",
    "multisphere"
};

static const size_t SHAPETYPE_NAME_COUNT = (sizeof(shapeTypeNames) / sizeof((shapeTypeNames)[0]));

// Bullet doesn't support arbitrarily small shapes
const float MIN_HALF_EXTENT = 0.005f; // 0.5 cm

QString ShapeInfo::getNameForShapeType(ShapeType type) {
    if (((int)type <= 0) || ((int)type >= (int)SHAPETYPE_NAME_COUNT)) {
        type = SHAPE_TYPE_NONE;
    }

    return shapeTypeNames[(int)type];
}

ShapeType ShapeInfo::getShapeTypeForName(QString string) {
    for (int i = 0; i < (int)SHAPETYPE_NAME_COUNT; i++) {
        auto name = shapeTypeNames[i];
        if (name == string) {
            return (ShapeType)i;
        }
    }

    return SHAPE_TYPE_NONE;
}

void ShapeInfo::clear() {
    _url.clear();
    _pointCollection.clear();
    _triangleIndices.clear();
    _sphereCollection.clear();
    _halfExtents = glm::vec3(0.0f);
    _offset = glm::vec3(0.0f);
    _hash64 = 0;
    _type = SHAPE_TYPE_NONE;
}

void ShapeInfo::setParams(ShapeType type, const glm::vec3& halfExtents, QString url) {
    _url = "";
    _type = type;
    setHalfExtents(halfExtents);
    switch(type) {
        case SHAPE_TYPE_NONE:
            _halfExtents = glm::vec3(0.0f);
            break;
        case SHAPE_TYPE_BOX:
        case SHAPE_TYPE_HULL:
        case SHAPE_TYPE_MULTISPHERE:
            break;
        case SHAPE_TYPE_CIRCLE: {
            _halfExtents = glm::vec3(_halfExtents.x, MIN_HALF_EXTENT, _halfExtents.z);
        }
        break;
        case SHAPE_TYPE_COMPOUND:
        case SHAPE_TYPE_SIMPLE_HULL:
        case SHAPE_TYPE_SIMPLE_COMPOUND:
        case SHAPE_TYPE_STATIC_MESH:
            _url = QUrl(url);
            break;
        default:
            break;
    }
    _hash64 = 0;
}

void ShapeInfo::setBox(const glm::vec3& halfExtents) {
    _url = "";
    _type = SHAPE_TYPE_BOX;
    setHalfExtents(halfExtents);
    _hash64 = 0;
}

void ShapeInfo::setSphere(float radius) {
    _url = "";
    _type = SHAPE_TYPE_SPHERE;
    radius = glm::max(radius, MIN_HALF_EXTENT);
    _halfExtents = glm::vec3(radius);
    _hash64 = 0;
}

void ShapeInfo::setMultiSphere(const std::vector<glm::vec3>& centers, const std::vector<float>& radiuses) {
    _url = "";
    _type = SHAPE_TYPE_MULTISPHERE;
    assert(centers.size() == radiuses.size());
    assert(centers.size() > 0);
    for (size_t i = 0; i < centers.size(); i++) {
        SphereData sphere = SphereData(centers[i], radiuses[i]);
        _sphereCollection.push_back(sphere);
    }
    _hash64 = 0;
}

void ShapeInfo::setPointCollection(const ShapeInfo::PointCollection& pointCollection) {
    _pointCollection = pointCollection;
    _hash64 = 0;
}

void ShapeInfo::setCapsuleY(float radius, float cylinderHalfHeight) {
    _url = "";
    _type = SHAPE_TYPE_CAPSULE_Y;
    radius = glm::max(radius, MIN_HALF_EXTENT);
    cylinderHalfHeight = glm::max(cylinderHalfHeight, 0.0f);
    _halfExtents = glm::vec3(radius, cylinderHalfHeight + radius, radius);
    _hash64 = 0;
}

void ShapeInfo::setOffset(const glm::vec3& offset) {
    _offset = offset;
    _hash64 = 0;
}

uint32_t ShapeInfo::getNumSubShapes() const {
    switch (_type) {
        case SHAPE_TYPE_NONE:
            return 0;
        case SHAPE_TYPE_COMPOUND:
        case SHAPE_TYPE_SIMPLE_COMPOUND:
            return _pointCollection.size();
        case SHAPE_TYPE_MULTISPHERE:
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

float computeCylinderVolume(const float radius, const float height) {
    return PI * radius * radius * 2.0f * height;
}

float computeCapsuleVolume(const float radius, const float cylinderHeight) {
    return PI * radius * radius * (cylinderHeight + 4.0f * radius / 3.0f);
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
        case SHAPE_TYPE_ELLIPSOID:
        case SHAPE_TYPE_SPHERE: {
            volume = 4.0f * PI * _halfExtents.x * _halfExtents.y * _halfExtents.z / 3.0f;
            break;
        }
        case SHAPE_TYPE_CYLINDER_X: {
            volume = computeCylinderVolume(_halfExtents.y, _halfExtents.x);
            break;
        }
        case SHAPE_TYPE_CYLINDER_Y: {
            volume = computeCylinderVolume(_halfExtents.x, _halfExtents.y);
            break;
        }
        case SHAPE_TYPE_CYLINDER_Z: {
            volume = computeCylinderVolume(_halfExtents.x, _halfExtents.z);
            break;
        }
        case SHAPE_TYPE_CAPSULE_X: {
            // Need to offset halfExtents.x by y to account for the system treating
            // the x extent of the capsule as the cylindrical height + spherical radius.
            const float cylinderHeight = 2.0f * (_halfExtents.x - _halfExtents.y);
            volume = computeCapsuleVolume(_halfExtents.y, cylinderHeight);
            break;
        }
        case SHAPE_TYPE_CAPSULE_Y: {
            // Need to offset halfExtents.y by x to account for the system treating
            // the y extent of the capsule as the cylindrical height + spherical radius.
            const float cylinderHeight = 2.0f * (_halfExtents.y - _halfExtents.x);
            volume = computeCapsuleVolume(_halfExtents.x, cylinderHeight);
            break;
        }
        case SHAPE_TYPE_CAPSULE_Z: {
            // Need to offset halfExtents.z by x to account for the system treating
            // the z extent of the capsule as the cylindrical height + spherical radius.
            const float cylinderHeight = 2.0f * (_halfExtents.z - _halfExtents.x);
            volume = computeCapsuleVolume(_halfExtents.x, cylinderHeight);
            break;
        }
        default:
            break;
    }
    assert(volume > 0.0f);
    return volume;
}

uint64_t ShapeInfo::getHash() const {
    // NOTE: we cache the key so we only ever need to compute it once for any valid ShapeInfo instance.
    if (_hash64 == 0 && _type != SHAPE_TYPE_NONE) {
        HashKey::Hasher hasher;
        // The key is not yet cached therefore we must compute it.

        hasher.hashUint64((uint64_t)_type);
        if (_type == SHAPE_TYPE_MULTISPHERE) {
            for (auto &sphereData : _sphereCollection) {
                hasher.hashVec3(glm::vec3(sphereData));
                hasher.hashFloat(sphereData.w);
            }
        } else if (_type != SHAPE_TYPE_SIMPLE_HULL) {
            hasher.hashVec3(_halfExtents);
            hasher.hashVec3(_offset);
        } else {
            // TODO: we could avoid hashing all of these points if we were to supply the ShapeInfo with a unique
            // descriptive string.  Shapes that are uniquely described by their type and URL could just put their
            // url in the description.
            assert(_pointCollection.size() == (size_t)1);
            const PointList & points = _pointCollection.back();
            const int numPoints = (int)points.size();

            for (int i = 0; i < numPoints; ++i) {
                hasher.hashVec3(points[i]);
            }
        }

        QString url = _url.toString();
        if (!url.isEmpty()) {
            QByteArray baUrl = url.toLocal8Bit();
            uint32_t urlHash = qChecksum(baUrl.data(), baUrl.size());
            hasher.hashUint64((uint64_t)urlHash);
        }

        if (_type == SHAPE_TYPE_COMPOUND || _type == SHAPE_TYPE_SIMPLE_COMPOUND) {
            uint64_t numHulls = (uint64_t)_pointCollection.size();
            hasher.hashUint64(numHulls);
        } else if (_type == SHAPE_TYPE_MULTISPHERE) {
            uint64_t numSpheres = (uint64_t)_sphereCollection.size();
            hasher.hashUint64(numSpheres);
        } else if (_type == SHAPE_TYPE_SIMPLE_HULL) {
            hasher.hashUint64(1);
        }
        _hash64 = hasher.getHash64();
    }
    return _hash64;
}

void ShapeInfo::setHalfExtents(const glm::vec3& halfExtents) {
    _halfExtents = glm::max(halfExtents, glm::vec3(MIN_HALF_EXTENT));
    _hash64 = 0;
}
