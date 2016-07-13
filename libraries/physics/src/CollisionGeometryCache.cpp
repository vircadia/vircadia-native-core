//
//  CollisionGeometryCache.cpp
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
#include "CollisionGeometryCache.h"

int foo = 0;

GeometryPointer createGeometryFromShape(CollisionGeometryCache::Key key) {
    return std::make_shared<int>(++foo);
}

CollisionGeometryCache::CollisionGeometryCache() {
}

CollisionGeometryCache::~CollisionGeometryCache() {
    _geometryMap.clear();
    _pendingGarbage.clear();
}

GeometryPointer CollisionGeometryCache::getGeometry(CollisionGeometryCache::Key key) {
    if (!key) {
        return GeometryPointer();
    }
    GeometryPointer geometry = 0;

    CollisionGeometryMap::const_iterator itr = _geometryMap.find(key);
    if (itr != _geometryMap.end()) {
        // make geometry and add it to map
        geometry = createGeometryFromShape(key);
        if (geometry) {
            _geometryMap.insert(std::make_pair(key, geometry));
        }
    }
    return geometry;
}

bool CollisionGeometryCache::releaseGeometry(CollisionGeometryCache::Key key) {
    if (!key) {
        return false;
    }
    CollisionGeometryMap::const_iterator itr = _geometryMap.find(key);
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

void CollisionGeometryCache::collectGarbage() {
    int numShapes = _pendingGarbage.size();
    for (int i = 0; i < numShapes; ++i) {
        CollisionGeometryCache::Key key = _pendingGarbage[i];
        CollisionGeometryMap::const_iterator itr = _geometryMap.find(key);
        if (itr != _geometryMap.end()) {
            if ((*itr).second.use_count() == 1) {
                // we hold the only reference
                _geometryMap.erase(itr);
            }
        }
    }
    _pendingGarbage.clear();
}

