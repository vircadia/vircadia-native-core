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

#include <SharedUtil.h> // for MILLIMETERS_PER_METER

#include "ShapeInfoUtil.h"
#include "BulletUtil.h"

int ShapeInfoUtil::toBulletShapeType(int shapeInfoType) {
    int bulletShapeType = INVALID_SHAPE_PROXYTYPE;
    switch(shapeInfoType) {
        case SHAPE_TYPE_BOX:
            bulletShapeType = BOX_SHAPE_PROXYTYPE;
            break;
        case SHAPE_TYPE_SPHERE:
            bulletShapeType = SPHERE_SHAPE_PROXYTYPE;
            break;
        case SHAPE_TYPE_CAPSULE_Y:
            bulletShapeType = CAPSULE_SHAPE_PROXYTYPE;
            break;
    }
    return bulletShapeType;
}

int ShapeInfoUtil::fromBulletShapeType(int bulletShapeType) {
    int shapeInfoType = SHAPE_TYPE_NONE;
    switch(bulletShapeType) {
        case BOX_SHAPE_PROXYTYPE:
            shapeInfoType = SHAPE_TYPE_BOX;
            break;
        case SPHERE_SHAPE_PROXYTYPE:
            shapeInfoType = SHAPE_TYPE_SPHERE;
            break;
        case CAPSULE_SHAPE_PROXYTYPE:
            shapeInfoType = SHAPE_TYPE_CAPSULE_Y;
            break;
    }
    return shapeInfoType;
}

void ShapeInfoUtil::collectInfoFromShape(const btCollisionShape* shape, ShapeInfo& info) {
    if (shape) {
        int type = ShapeInfoUtil::fromBulletShapeType(shape->getShapeType());
        switch(type) {
            case SHAPE_TYPE_BOX: {
                const btBoxShape* boxShape = static_cast<const btBoxShape*>(shape);
                info.setBox(bulletToGLM(boxShape->getHalfExtentsWithMargin()));
            }
            break;
            case SHAPE_TYPE_SPHERE: {
                const btSphereShape* sphereShape = static_cast<const btSphereShape*>(shape);
                info.setSphere(sphereShape->getRadius());
            }
            break;
            default:
                info.clear();
            break;
        }
    } else {
        info.clear();
    }
}

btCollisionShape* ShapeInfoUtil::createShapeFromInfo(const ShapeInfo& info) {
    btCollisionShape* shape = NULL;
    switch(info.getType()) {
        case SHAPE_TYPE_BOX: {
            shape = new btBoxShape(glmToBullet(info.getHalfExtents()));
        }
        break;
        case SHAPE_TYPE_SPHERE: {
            float radius = info.getHalfExtents().x;
            shape = new btSphereShape(radius);
        }
        break;
        case SHAPE_TYPE_CAPSULE_Y: {
            glm::vec3 halfExtents = info.getHalfExtents();
            float radius = halfExtents.x;
            float height = 2.0f * halfExtents.y;
            shape = new btCapsuleShape(radius, height);
        }
        break;
    }
    return shape;
}

/*
const DoubleHashKey& ShapeInfo::computeHash() {
    if (_hash.isNull()) {
        // compute hash1
        // TODO?: provide lookup table for hash/hash2 of _type rather than recompute?
        int primeIndex = 0;
        _doubleHashKey._hash = DoubleHashKey::hashFunction((uint32_t)_type, primeIndex++);
        _doubleHashKey._hash2 = DoubleHashKey::hashFunction2((uint32_t)_type());
    
        if (getData()) {
            // if externalData exists we use that to continue the hash

            // compute hash
            uint32_t hash = _doubleHashKey._hash;
            const QVector<glm::vec3>& data = getData();
    
            glm::vec3 tmpData;
            int numData = data.size();
            for (int i = 0; i < numData; ++i) {
                tmpData = data[i];
                for (int j = 0; j < 3; ++j) {
                    // NOTE: 0.49f is used to bump the float up almost half a millimeter
                    // so the cast to int produces a round() effect rather than a floor()
                    uint32_t floatHash =
                        DoubleHashKey::hashFunction((uint32_t)(tmpData[j] * MILLIMETERS_PER_METER + copysignf(1.0f, tmpData[j]) * 0.49f), primeIndex++);
                    hash ^= floatHash;
                }
            }
            _doubleHashKey._hash = (int32_t)hash;
        
            // compute hash2
            hash = _doubleHashKey._hash2;
            for (int i = 0; i < numData; ++i) {
                tmpData = data[i];
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
            _doubleHashKey._hash2 = (uint32_t)hash;
        } else {
            // this shape info has no external data so we just use extents to continue hash
            // compute hash1
            uint32_t hash = _doubleHashKey._hash;
            for (int j = 0; j < 3; ++j) {
                // NOTE: 0.49f is used to bump the float up almost half a millimeter
                // so the cast to int produces a round() effect rather than a floor()
                uint32_t floatHash =
                    DoubleHashKey::hashFunction((uint32_t)(_halfExtents[j] * MILLIMETERS_PER_METER + copysignf(1.0f, _halfExtents[j]) * 0.49f), primeIndex++);
                hash ^= floatHash;
            }
            _doubleHashKey._hash = (uint32_t)hash;
        
            // compute hash2
            hash = _doubleHashKey._hash2;
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
            _doubleHashKey._hash2 = (uint32_t)hash;
        }
    }
        
    return _doubleHashKey;
}
*/
