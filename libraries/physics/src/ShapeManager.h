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

#include <atomic>
#include <vector>

#include <QObject>
#include <btBulletDynamicsCommon.h>
#include <LinearMath/btHashMap.h>

#include <ShapeInfo.h>

#include "ShapeFactory.h"
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


class ShapeManager : public QObject {
    Q_OBJECT
public:

    ShapeManager();
    ~ShapeManager();

    /// \return pointer to shape
    const btCollisionShape* getShape(const ShapeInfo& info);
    const btCollisionShape* getShapeByKey(uint64_t key);
    bool hasShapeWithKey(uint64_t key) const;

    /// \return true if shape was found and released
    bool releaseShape(const btCollisionShape* shape);

    /// delete shapes that have zero references
    void collectGarbage();

    // validation methods
    int getNumShapes() const { return _shapeMap.size(); }
    int getNumReferences(const ShapeInfo& info) const;
    int getNumReferences(const btCollisionShape* shape) const;
    bool hasShape(const btCollisionShape* shape) const;
    uint32_t getWorkRequestCount() const { return _workRequestCount; }
    uint32_t getWorkDeliveryCount() const { return _workDeliveryCount; }

protected slots:
    void acceptWork(ShapeFactory::Worker* worker);

private:
    void addToGarbage(uint64_t key);
    bool releaseShapeByKey(uint64_t key);

    class ShapeReference {
    public:
        int refCount;
        const btCollisionShape* shape;
        uint64_t key { 0 };
        ShapeReference() : refCount(0), shape(nullptr) {}
    };

    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    class KeyExpiry {
    public:
        KeyExpiry(uint64_t k, std::chrono::time_point<std::chrono::steady_clock> e) : expiry(e), key(k) {}
        TimePoint expiry;
        uint64_t key;
    };

    // btHashMap is required because it supports memory alignment of the btCollisionShapes
    btHashMap<HashKey, ShapeReference> _shapeMap;
    std::vector<uint64_t> _garbageRing;
    std::vector<uint64_t> _pendingMeshShapes;
    std::vector<KeyExpiry> _orphans;
    ShapeFactory::Worker* _deadWorker { nullptr };
    TimePoint _nextOrphanExpiry;
    uint32_t _ringIndex { 0 };
    std::atomic_uint _workRequestCount { 0 };
    std::atomic_uint _workDeliveryCount { 0 };
};

#endif // hifi_ShapeManager_h
