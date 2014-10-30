//
//  ShapeManager.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShapeManager_h
#define hifi_ShapeManager_h

#ifdef USE_BULLET_PHYSICS

#include <btBulletDynamicsCommon.h>
#include <LinearMath/btHashMap.h>

struct ShapeInfo {
    int _type;
    btVector3 _size;

    ShapeInfo() : _type(BOX_SHAPE_PROXYTYPE), _size(1.0f, 1.0f, 1.0f) {}

    virtual int getHash() const {
        // successfully multiply components of size by primes near 256 and convert to U32
        unsigned int key = (unsigned int)(241.0f * _size.getX()) 
            + 241 * (unsigned int)(251.0f * _size.getY()) 
            + (241 * 251) * (unsigned int)(257.0f * _size.getZ());
        // then scramble the results
        key += ~(key << 15);
        key ^=  (key >> 10);
        key +=  (key << 3); 
        key ^=  (key >> 6); 
        key += ~(key << 11);
        key ^=  (key >> 16);
        // finally XOR with type
        key ^= _type;
        return key;
    }

    virtual btCollisionShape* createShape() const {
        const float MAX_SHAPE_DIMENSION = 100.0f;
        const float MIN_SHAPE_DIMENSION = 0.01f;
        for (int i = 0; i < 3; ++i) {
            float side = _size[i];
            if (side > MAX_SHAPE_DIMENSION || side < MIN_SHAPE_DIMENSION) {
                return NULL;
            }
        }
        // default behavior is to create a btBoxShape
        return new btBoxShape(0.5f * _size);
    }
};

struct ShapeReference {
    int _refCount;
    btCollisionShape* _shape;
    ShapeReference() : _refCount(0), _shape(NULL) {}
};

class ShapeManager {
public:

    ShapeManager();
    ~ShapeManager();

    /// \return pointer to shape
    btCollisionShape* getShape(const ShapeInfo& info);

    /// \return true if shape was found and released
    bool releaseShape(const ShapeInfo& info);

    /// delete shapes that have zero references
    void collectGarbage();

    // validation methods
    int getNumShapes() const { return _shapeMap.size(); }
    int getNumReferences(const ShapeInfo& info) const;

private:
    btHashMap<btHashInt, ShapeReference> _shapeMap;
    btAlignedObjectArray<int> _pendingGarbage;
};

#endif // USE_BULLET_PHYSICS
#endif // hifi_ShapeManager_h
