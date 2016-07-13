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

#include "CollisionRenderMeshCache.h"

#include <cassert>
//#include <glm/gtx/norm.hpp>

//#include "ShapeFactory.h"
#include <btBulletDynamicsCommon.h>


model::MeshPointer createMeshFromShape(const btCollisionShape* shape) {
    if (!shape) {
        return std::make_shared<model::Mesh>();
    }
    int32_t shapeType = shape->getShapeType();
    if (shapeType == (int32_t)COMPOUND_SHAPE_PROXYTYPE) {
        const btCompoundShape* compoundShape = static_cast<const btCompoundShape*>(shape);
        int32_t numSubShapes = compoundShape->getNumChildShapes();
        for (int i = 0; i < numSubShapes; ++i) {
            const btCollisionShape* childShape = compoundShape->getChildShape(i);
            std::cout << "adebug " << i << "  " << (void*)(childShape) << std::endl;  // adebug
        }
    } else if (shape->isConvex()) {
        std::cout << "adebug " << (void*)(shape)<< std::endl;  // adebug
    }
    return std::make_shared<model::Mesh>();
}

CollisionRenderMeshCache::CollisionRenderMeshCache() {
}

CollisionRenderMeshCache::~CollisionRenderMeshCache() {
    _geometryMap.clear();
    _pendingGarbage.clear();
}

model::MeshPointer CollisionRenderMeshCache::getMesh(CollisionRenderMeshCache::Key key) {
    model::MeshPointer mesh;
    if (key) {
        CollisionMeshMap::const_iterator itr = _geometryMap.find(key);
        if (itr != _geometryMap.end()) {
            // make mesh and add it to map
            mesh = createMeshFromShape(key);
            if (mesh) {
                _geometryMap.insert(std::make_pair(key, mesh));
            }
        }
    }
    return mesh;
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

