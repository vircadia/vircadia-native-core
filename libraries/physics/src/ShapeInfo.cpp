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

// prime numbers larger than 1e6
const int NUM_PRIMES = 64;
const unsigned int PRIMES[] = { 
    4194301U, 4194287U, 4194277U, 4194271U, 4194247U, 4194217U, 4194199U, 4194191U,
    4194187U, 4194181U, 4194173U, 4194167U, 4194143U, 4194137U, 4194131U, 4194107U,
    4194103U, 4194023U, 4194011U, 4194007U, 4193977U, 4193971U, 4193963U, 4193957U,
    4193939U, 4193929U, 4193909U, 4193869U, 4193807U, 4193803U, 4193801U, 4193789U,
    4193759U, 4193753U, 4193743U, 4193701U, 4193663U, 4193633U, 4193573U, 4193569U,
    4193551U, 4193549U, 4193531U, 4193513U, 4193507U, 4193459U, 4193447U, 4193443U,
    4193417U, 4193411U, 4193393U, 4193389U, 4193381U, 4193377U, 4193369U, 4193359U,
    4193353U, 4193327U, 4193309U, 4193303U, 4193297U, 4193279U, 4193269U, 4193263U
};

void ShapeInfo::collectInfo(const btCollisionShape* shape) {
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

unsigned int ShapeInfo::hashFunction(unsigned int value, int primeIndex) {
    unsigned int hash = PRIMES[primeIndex % NUM_PRIMES] * (value + 1U);
    hash += ~(hash << 15);
    hash ^=  (hash >> 10);
    hash +=  (hash << 3); 
    hash ^=  (hash >> 6); 
    hash += ~(hash << 11);
    return hash ^ (hash >> 16);
}

unsigned int ShapeInfo::hashFunction2(unsigned int value) {
    unsigned hash = 0x811c9dc5U;
    for (int i = 0; i < 4; i++ ) {
        unsigned int byte = (value << (i * 8)) >> (24 - i * 8);
        hash = ( hash ^ byte ) * 0x01000193U;
    }
   return hash;
}

const float MILLIMETERS_PER_METER = 1000.0f;

int ShapeInfo::computeHash() const {
    // scramble the bits of the type
    // TODO?: provide lookup table for hash of _type?
    int primeIndex = 0;
    unsigned int hash = ShapeInfo::hashFunction((unsigned int)_type, primeIndex++);

    btVector3 tmpData;
    int numData = _data.size();
    for (int i = 0; i < numData; ++i) {
        tmpData = _data[i];
        for (int j = 0; j < 3; ++j) {
            // multiply these mm by a new prime
            unsigned int floatHash = ShapeInfo::hashFunction((unsigned int)(tmpData[j] * MILLIMETERS_PER_METER + 0.49f), primeIndex++);
            hash ^= floatHash;
        }
    }
    return hash;
}

int ShapeInfo::computeHash2() const {
    // scramble the bits of the type
    // TODO?: provide lookup table for hash of _type?
    unsigned int hash = ShapeInfo::hashFunction2((unsigned int)_type);

    btVector3 tmpData;
    int numData = _data.size();
    for (int i = 0; i < numData; ++i) {
        tmpData = _data[i];
        for (int j = 0; j < 3; ++j) {
            unsigned int floatHash = ShapeInfo::hashFunction2((unsigned int)(tmpData[j] * MILLIMETERS_PER_METER + 0.49f));
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

btCollisionShape* ShapeInfo::createShape() const {
    btCollisionShape* shape = NULL;
    int numData = _data.size();
    switch(_type) {
        case BOX_SHAPE_PROXYTYPE: {
            if (numData > 0) {
                btVector3 halfExtents = _data[0];
                shape = new btBoxShape(halfExtents);
            }
        }
        break;
        case SPHERE_SHAPE_PROXYTYPE: {
            if (numData > 0) {
                float radius = _data[0].getZ();
                shape = new btSphereShape(radius);
            }
        }
        break;
        case CYLINDER_SHAPE_PROXYTYPE: {
            if (numData > 0) {
                btVector3 halfExtents = _data[0];
                // NOTE: default cylinder has (UpAxis = 1) axis along yAxis and radius stored in X
                // halfExtents = btVector3(radius, halfHeight, unused)
                shape = new btCylinderShape(halfExtents);
            }
        }
        break;
        case CAPSULE_SHAPE_PROXYTYPE: {
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
