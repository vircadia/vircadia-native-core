//
//  ShapeInfoUtil.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.12.01
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <Shape.h> // for FOO_SHAPE types
#include <SharedUtil.h> // for MILLIMETERS_PER_METER

#include "ShapeInfoUtil.h"
#include "BulletUtil.h"

#ifdef USE_BULLET_PHYSICS


int ShapeInfoUtil::toBulletShapeType(int shapeInfoType) {
    int bulletShapeType = INVALID_SHAPE_PROXYTYPE;
    switch(shapeInfoType) {
        case BOX_SHAPE:
            bulletShapeType = BOX_SHAPE_PROXYTYPE;
            break;
        case SPHERE_SHAPE:
            bulletShapeType = SPHERE_SHAPE_PROXYTYPE;
            break;
        case CAPSULE_SHAPE:
            bulletShapeType = CAPSULE_SHAPE_PROXYTYPE;
            break;
        case CYLINDER_SHAPE:
            bulletShapeType = CYLINDER_SHAPE_PROXYTYPE;
            break;
    }
    return bulletShapeType;
}

int ShapeInfoUtil::fromBulletShapeType(int bulletShapeType) {
    int shapeInfoType = INVALID_SHAPE;
    switch(bulletShapeType) {
        case BOX_SHAPE_PROXYTYPE:
            shapeInfoType = BOX_SHAPE;
            break;
        case SPHERE_SHAPE_PROXYTYPE:
            shapeInfoType = SPHERE_SHAPE;
            break;
        case CAPSULE_SHAPE_PROXYTYPE:
            shapeInfoType = CAPSULE_SHAPE;
            break;
        case CYLINDER_SHAPE_PROXYTYPE:
            shapeInfoType = CYLINDER_SHAPE;
            break;
    }
    return shapeInfoType;
}

void ShapeInfoUtil::collectInfoFromShape(const btCollisionShape* shape, ShapeInfo& info) {
    info._data.clear();
    if (shape) {
        info._type = ShapeInfoUtil::fromBulletShapeType(shape->getShapeType());
        switch(info._type) {
            case BOX_SHAPE:
            {
                const btBoxShape* boxShape = static_cast<const btBoxShape*>(shape);
                glm::vec3 halfExtents;
                bulletToGLM(boxShape->getHalfExtentsWithMargin(), halfExtents);
                info._data.push_back(halfExtents);
            }
            break;
            case SPHERE_SHAPE:
            {
                const btSphereShape* sphereShape = static_cast<const btSphereShape*>(shape);
                glm::vec3 data;
                bulletToGLM(btVector3(0.0f, 0.0f, sphereShape->getRadius()), data);
                info._data.push_back(data);
            }
            break;
            case CYLINDER_SHAPE:
            {
                const btCylinderShape* cylinderShape = static_cast<const btCylinderShape*>(shape);
                glm::vec3 halfExtents;
                bulletToGLM(cylinderShape->getHalfExtentsWithMargin(), halfExtents);
                info._data.push_back(halfExtents);
            }
            break;
            case CAPSULE_SHAPE:
            {
                const btCapsuleShape* capsuleShape = static_cast<const btCapsuleShape*>(shape);
                glm::vec3 data;
                bulletToGLM(btVector3(capsuleShape->getRadius(), capsuleShape->getHalfHeight(), 0.0f), data);
                info._data.push_back(data);
                // NOTE: we only support capsules with axis along yAxis
            }
            break;
            default:
                info._type = INVALID_SHAPE;
            break;
        }
    } else {
        info._type = INVALID_SHAPE;
    }
}

btCollisionShape* ShapeInfoUtil::createShapeFromInfo(const ShapeInfo& info) {
    btCollisionShape* shape = NULL;
    int numData = info._data.size();
    switch(info._type) {
        case BOX_SHAPE: {
            if (numData > 0) {
                btVector3 halfExtents;
                glmToBullet(info._data[0], halfExtents);
                shape = new btBoxShape(halfExtents);
            }
        }
        break;
        case SPHERE_SHAPE: {
            if (numData > 0) {
                float radius = info._data[0].z;
                shape = new btSphereShape(radius);
            }
        }
        break;
        case CYLINDER_SHAPE: {
            if (numData > 0) {
                btVector3 halfExtents;
                glmToBullet(info._data[0], halfExtents);
                // NOTE: default cylinder has (UpAxis = 1) axis along yAxis and radius stored in X
                // halfExtents = btVector3(radius, halfHeight, unused)
                shape = new btCylinderShape(halfExtents);
            }
        }
        break;
        case CAPSULE_SHAPE: {
            if (numData > 0) {
                float radius = info._data[0].x;
                float height = 2.0f * info._data[0].y;
                shape = new btCapsuleShape(radius, height);
            }
        }
        break;
    }
    return shape;
}

DoubleHashKey ShapeInfoUtil::computeHash(const ShapeInfo& info) {
    DoubleHashKey key;
    // compute hash
    // scramble the bits of the type
    // TODO?: provide lookup table for hash of info._type rather than recompute?
    int primeIndex = 0;
    unsigned int hash = DoubleHashKey::hashFunction((unsigned int)info._type, primeIndex++);

    glm::vec3 tmpData;
    int numData = info._data.size();
    for (int i = 0; i < numData; ++i) {
        tmpData = info._data[i];
        for (int j = 0; j < 3; ++j) {
            // NOTE: 0.49f is used to bump the float up almost half a millimeter
            // so the cast to int produces a round() effect rather than a floor()
            unsigned int floatHash =
                DoubleHashKey::hashFunction((int)(tmpData[j] * MILLIMETERS_PER_METER + copysignf(1.0f, tmpData[j]) * 0.49f), primeIndex++);
            hash ^= floatHash;
        }
    }
    key._hash = (int)hash;

    // compute hash2
    // scramble the bits of the type
    // TODO?: provide lookup table for hash2 of info._type rather than recompute?
    hash = DoubleHashKey::hashFunction2((unsigned int)info._type);

    for (int i = 0; i < numData; ++i) {
        tmpData = info._data[i];
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
    key._hash2 = (int)hash;
    return key;
}

#endif // USE_BULLET_PHYSICS
