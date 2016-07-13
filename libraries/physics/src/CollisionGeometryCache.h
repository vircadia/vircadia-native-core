//
//  CollisionGeometryCache.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2016.07.13
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CollisionGeometryCache
#define hifi_CollisionGeometryCache

#include <memory>
#include <vector>
#include <unordered_map>
//#include <btBulletDynamicsCommon.h>
//#include <LinearMath/btHashMap.h>

class btCollisionShape;

// BEGIN TEST HACK
using GeometryPointer = std::shared_ptr<int>;
// END TEST HACK

namespace std {
    template <>
    struct hash<btCollisionShape*> {
        std::size_t operator()(btCollisionShape* key) const {
            return (hash<void*>()((void*)key));
        }
    };
}

class CollisionGeometryCache {
public:
	using Key = btCollisionShape const *;

    CollisionGeometryCache();
    ~CollisionGeometryCache();

    /// \return pointer to geometry
    GeometryPointer getGeometry(Key key);

    /// \return true if geometry was found and released
    bool releaseGeometry(Key key);

    /// delete geometries that have zero references
    void collectGarbage();

    // validation methods
    uint32_t getNumGeometries() const { return (uint32_t)_geometryMap.size(); }
    bool hasGeometry(Key key) const { return _geometryMap.find(key) == _geometryMap.end(); }

private:
    using CollisionGeometryMap = std::unordered_map<Key, GeometryPointer>;
    CollisionGeometryMap _geometryMap;
    std::vector<Key> _pendingGarbage;
};

#endif // hifi_CollisionGeometryCache
