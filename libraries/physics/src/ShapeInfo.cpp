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

#include <btBulletDynamicsCommon.h>

#include "ShapeInfo.h"

void ShapeInfo::getInfo(const btCollisionShape* shape) {
    _data.clear();
    if (shape) {
        _type = (unsigned int)(shape->getShapeType());
        switch(_type) {
            case BOX_SHAPE_PROXYTYPE:
            {
                const btBoxShape* boxShape = static_cast<const btBoxShape*>(shape);
                _data.push_back(boxShape->getHalfExtentsWithMargin());
            }
            break;
            case SPHERE_SHAPE_PROXYTYPE:
            {
                const btSphereShape* sphereShape = static_cast<const btSphereShape*>(shape);
                _data.push_back(btVector3(0.0f, 0.0f, sphereShape->getRadius()));
            }
            break;
            case CYLINDER_SHAPE_PROXYTYPE:
            {
                const btCylinderShape* cylinderShape = static_cast<const btCylinderShape*>(shape);
                _data.push_back(cylinderShape->getHalfExtentsWithMargin());
            }
            break;
            case CAPSULE_SHAPE_PROXYTYPE:
            {
                const btCapsuleShape* capsuleShape = static_cast<const btCapsuleShape*>(shape);
                _data.push_back(btVector3(capsuleShape->getRadius(), capsuleShape->getHalfHeight(), 0.0f));
                // NOTE: we only support capsules with axis along yAxis
            }
            break;
            default:
                _type = INVALID_SHAPE_PROXYTYPE;
            break;
        }
    } else {
        _type = INVALID_SHAPE_PROXYTYPE;
    }
}

void ShapeInfo::setBox(const btVector3& halfExtents) {
    _type = BOX_SHAPE_PROXYTYPE;
    _data.clear();
    _data.push_back(halfExtents);
}

void ShapeInfo::setSphere(float radius) {
    _type = SPHERE_SHAPE_PROXYTYPE;
    _data.clear();
    _data.push_back(btVector3(0.0f, 0.0f, radius));
}

void ShapeInfo::setCylinder(float radius, float height) {
    _type = CYLINDER_SHAPE_PROXYTYPE;
    _data.clear();
    // NOTE: default cylinder has (UpAxis = 1) axis along yAxis and radius stored in X
    // halfExtents = btVector3(radius, halfHeight, unused)
    _data.push_back(btVector3(radius, 0.5f * height, radius));
}

void ShapeInfo::setCapsule(float radius, float height) {
    _type = CAPSULE_SHAPE_PROXYTYPE;
    _data.clear();
    _data.push_back(btVector3(radius, 0.5f * height, 0.0f));
}

int ShapeInfo::computeHash() const {
    // This hash algorithm works well for shapes that have dimensions less than about 256m.
    // At larger values the likelihood of hash collision goes up because of the most 
    // significant bits are pushed off the top and the result could be the same as for smaller
    // dimensions (truncation).
    //
    // The algorithm may produce collisions for shapes whose dimensions differ by less than 
    // ~1/256 m, however this is by design -- we don't expect collision differences smaller
    // than 1 mm to be noticable.
    unsigned int key = 0;
    btVector3 tempData;
    int numData = _data.size();
    for (int i = 0; i < numData; ++i) {
        // Successively multiply components of each vec3 by primes near 512 and convert to U32
        // to spread the data across more bits.  Note that all dimensions are at half-value
        // (half extents, radius, etc) which is why we multiply by primes near 512 rather
        // than 256.
        tempData = _data[i];
        key += (unsigned int)(509.0f * (tempData.getZ() + 0.01f)) 
            + 509 * (unsigned int)(521.0f * (tempData.getY() + 0.01f)) 
            + (509 * 521) * (unsigned int)(523.0f * (tempData.getX() + 0.01f));
        // avalanch the bits
        key += ~(key << 15);
        key ^=  (key >> 10);
        key +=  (key << 3); 
        key ^=  (key >> 6); 
        key += ~(key << 11);
        key ^=  (key >> 16);
    }
    // finally XOR with type
    return (int)(key ^ _type);
}

btCollisionShape* ShapeInfo::createShape() const {
    btCollisionShape* shape = NULL;
    int numData = _data.size();
    switch(_type) {
        case BOX_SHAPE_PROXYTYPE:
        {
            if (numData > 0) {
                btVector3 halfExtents = _data[0];
                shape = new btBoxShape(halfExtents);
            }
        }
        break;
        case SPHERE_SHAPE_PROXYTYPE:
        {
            if (numData > 0) {
                float radius = _data[0].getZ();
                shape = new btSphereShape(radius);
            }
        }
        break;
        case CYLINDER_SHAPE_PROXYTYPE:
        {
            if (numData > 0) {
                btVector3 halfExtents = _data[0];
                // NOTE: default cylinder has (UpAxis = 1) axis along yAxis and radius stored in X
                // halfExtents = btVector3(radius, halfHeight, unused)
                shape = new btCylinderShape(halfExtents);
            }
        }
        break;
        case CAPSULE_SHAPE_PROXYTYPE:
        {
            if (numData > 0) {
                float radius = _data[0].getX();
                float height = 2.0f * _data[0].getY();
                shape = new btCapsuleShape(radius, height);
            }
        }
        break;
    }
    return shape;
}

#endif // USE_BULLET_PHYSICS
