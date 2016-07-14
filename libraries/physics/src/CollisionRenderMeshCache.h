//
//  CollisionRenderMeshCache.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2016.07.13
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CollisionRenderMeshCache_h
#define hifi_CollisionRenderMeshCache_h

#include <memory>
#include <vector>
#include <unordered_map>

#include <model/Geometry.h>

class btCollisionShape;

namespace std {
    template <>
    struct hash<btCollisionShape*> {
        std::size_t operator()(btCollisionShape* key) const {
            return (hash<void*>()((void*)key));
        }
    };
}

class CollisionRenderMeshCache {
public:
	using Key = const btCollisionShape*;

    CollisionRenderMeshCache();
    ~CollisionRenderMeshCache();

    /// \return pointer to geometry
    model::MeshPointer getMesh(Key key);

    /// \return true if geometry was found and released
    bool releaseMesh(Key key);

    /// delete geometries that have zero references
    void collectGarbage();

    // validation methods
    uint32_t getNumGeometries() const { return (uint32_t)_geometryMap.size(); }
    bool hasMesh(Key key) const { return _geometryMap.find(key) == _geometryMap.end(); }

private:
    using CollisionMeshMap = std::unordered_map<Key, model::MeshPointer>;
    CollisionMeshMap _geometryMap;
    std::vector<Key> _pendingGarbage;
};

#endif // hifi_CollisionRenderMeshCache_h
