//
//  ShapeManager.h
//  libraries/physics/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShapeManager_h
#define hifi_ShapeManager_h

#include <btBulletDynamicsCommon.h>
#include <LinearMath/btHashMap.h>

#include <ShapeInfo.h>

#include "HashKey.h"

// The ShapeManager handles the ref-counting on shared shapes:
//
// Each object added to the physics simulation gets a corresponding btRigidBody.
// The body has a btCollisionShape that represents the contours of its collision
// surface.  Multiple bodies may have the same shape.  Rather than create a unique
// btCollisionShape instance for every body with a particular shape we can instead
// use a single shape instance for all of the bodies.  This is called "shape
// sharing".
//
// When body needs a new shape a description of ths shape (ShapeInfo) is assembled
// and a request is sent to the ShapeManager for a corresponding btCollisionShape
// pointer.  The ShapeManager will compute a hash of the ShapeInfo's data and use
// that to find the shape in its map.  If it finds one it increments the ref-count
// and returns the pointer.  If not it asks the ShapeFactory to create it, adds an
// entry in the map with a ref-count of 1, and returns the pointer.
//
// When a body stops using a shape the ShapeManager must be informed so it can
// decrement its ref-count.  When a ref-count drops to zero the ShapeManager
// doesn't delete it right away.  Instead it puts the shape's key on a list delete
// later.  When that list grows big enough the ShapeManager will remove any matching
// entries that still have zero ref-count.

class ShapeManager {
public:

    ShapeManager();
    ~ShapeManager();

    /// \return pointer to shape
    const btCollisionShape* getShape(const ShapeInfo& info);

    /// \return true if shape was found and released
    bool releaseShape(const btCollisionShape* shape);

    /// delete shapes that have zero references
    void collectGarbage();

    // validation methods
    int getNumShapes() const { return _shapeMap.size(); }
    int getNumReferences(const ShapeInfo& info) const;
    int getNumReferences(const btCollisionShape* shape) const;
    bool hasShape(const btCollisionShape* shape) const;

private:
    bool releaseShapeByKey(const HashKey& key);

    class ShapeReference {
    public:
        int refCount;
        const btCollisionShape* shape;
        HashKey key;
        ShapeReference() : refCount(0), shape(nullptr) {}
    };

    // btHashMap is required because it supports memory alignment of the btCollisionShapes
    btHashMap<HashKey, ShapeReference> _shapeMap;
    btAlignedObjectArray<HashKey> _pendingGarbage;
};

#endif // hifi_ShapeManager_h
