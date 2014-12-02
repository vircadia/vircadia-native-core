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

#include <ShapeInfo.h>

#include "DoubleHashKey.h"

class ShapeManager {
public:

    ShapeManager();
    ~ShapeManager();

    /// \return pointer to shape
    btCollisionShape* getShape(const ShapeInfo& info);

    /// \return true if shape was found and released
    bool releaseShape(const ShapeInfo& info);
    bool releaseShape(const btCollisionShape* shape);

    /// delete shapes that have zero references
    void collectGarbage();

    // validation methods
    int getNumShapes() const { return _shapeMap.size(); }
    int getNumReferences(const ShapeInfo& info) const;

private:
    struct ShapeReference {
        int _refCount;
        btCollisionShape* _shape;
        ShapeReference() : _refCount(0), _shape(NULL) {}
    };

    btHashMap<DoubleHashKey, ShapeReference> _shapeMap;
    btAlignedObjectArray<DoubleHashKey> _pendingGarbage;
};

#endif // USE_BULLET_PHYSICS
#endif // hifi_ShapeManager_h
