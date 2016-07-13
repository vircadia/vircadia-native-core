//
//  CollisionRenderMeshCache.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2016.07.13
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cassert>
//#include <glm/gtx/norm.hpp>

//#include "ShapeFactory.h"
#include "CollisionRenderMeshCache.h"

int foo = 0;

MeshPointer createMeshFromShape(CollisionRenderMeshCache::Key key) {
    return std::make_shared<int>(++foo);
}

CollisionRenderMeshCache::CollisionRenderMeshCache() {
}

CollisionRenderMeshCache::~CollisionRenderMeshCache() {
    _geometryMap.clear();
    _pendingGarbage.clear();
}

MeshPointer CollisionRenderMeshCache::getMesh(CollisionRenderMeshCache::Key key) {
    if (!key) {
        return MeshPointer();
    }
    MeshPointer geometry = 0;

    CollisionMeshMap::const_iterator itr = _geometryMap.find(key);
    if (itr != _geometryMap.end()) {
        // make geometry and add it to map
        geometry = createMeshFromShape(key);
        if (geometry) {
            _geometryMap.insert(std::make_pair(key, geometry));
        }
    }
    return geometry;
}

bool CollisionRenderMeshCache::releaseMesh(CollisionRenderMeshCache::Key key) {
    if (!key) {
        return false;
    }
    CollisionMeshMap::const_iterator itr = _geometryMap.find(key);
    if (itr != _geometryMap.end()) {
        assert((*itr).second.use_count() != 1);
        if ((*itr).second.use_count() == 2) {
            // we hold all of the references inside the cache so we'll try to delete later
            _pendingGarbage.push_back(key);
        }
        return true;
    }
    return false;
}

void CollisionRenderMeshCache::collectGarbage() {
    int numShapes = _pendingGarbage.size();
    for (int i = 0; i < numShapes; ++i) {
        CollisionRenderMeshCache::Key key = _pendingGarbage[i];
        CollisionMeshMap::const_iterator itr = _geometryMap.find(key);
        if (itr != _geometryMap.end()) {
            if ((*itr).second.use_count() == 1) {
                // we hold the only reference
                _geometryMap.erase(itr);
            }
        }
    }
    _pendingGarbage.clear();
}

