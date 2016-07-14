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

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>

#include <ShapeInfo.h> // for MAX_HULL_POINTS

float verts[3 * MAX_HULL_POINTS];

void copyShapeToMesh(const btTransform& transform, const btConvexShape* shape, model::MeshPointer mesh) {
    assert((bool)mesh);
    assert(shape);

    btShapeHull hull(shape);
    const btScalar MARGIN = 0.0f;
    hull.buildHull(MARGIN);

    const uint32_t* hullIndices = hull.getIndexPointer();
    int32_t numIndices = hull.numIndices();
    assert(numIndices <= 6 * MAX_HULL_POINTS);

    { // new part
        model::Mesh::Part part;
        part._startIndex = mesh->getIndexBuffer().getNumElements();
        part._numIndices = (model::Index)numIndices;
        part._baseVertex = mesh->getVertexBuffer().getNumElements();

        gpu::BufferView::Size numBytes = sizeof(model::Mesh::Part);
        const gpu::Byte* data = reinterpret_cast<const gpu::Byte*>(&part);
        mesh->getPartBuffer()._buffer->append(numBytes, data);
    }

    { // new vertices
        const btVector3* hullVertices = hull.getVertexPointer();
        int32_t numVertices = hull.numVertices();
        assert(numVertices <= MAX_HULL_POINTS);
        for (int32_t i = 0; i < numVertices; ++i) {
            btVector3 transformedPoint = transform * hullVertices[i];
            memcpy(transformedPoint.m_floats, verts + 3 * i, 3 * sizeof(float));
            //data[0] = transformedPoint.getX();
            //data[1] = transformedPoint.getY();
            //data[2] = transformedPoint.getZ();
        }

        gpu::BufferView::Size numBytes = sizeof(float) * (3 * numVertices);
        const gpu::Byte* data = reinterpret_cast<const gpu::Byte*>(verts);
        mesh->getVertexBuffer()._buffer->append(numBytes, data);
    }

    { // new indices
        gpu::BufferView::Size numBytes = (gpu::BufferView::Size)(sizeof(uint32_t) * hull.numIndices());
        const gpu::Byte* data = reinterpret_cast<const gpu::Byte*>(hullIndices);
        mesh->getIndexBuffer()._buffer->append(numBytes, data);
    }
}

model::MeshPointer createMeshFromShape(const btCollisionShape* shape) {
    model::MeshPointer mesh = std::make_shared<model::Mesh>();
    if (shape) {
        int32_t shapeType = shape->getShapeType();
        if (shapeType == (int32_t)COMPOUND_SHAPE_PROXYTYPE) {
            const btCompoundShape* compoundShape = static_cast<const btCompoundShape*>(shape);
            int32_t numSubShapes = compoundShape->getNumChildShapes();
            for (int i = 0; i < numSubShapes; ++i) {
                const btCollisionShape* childShape = compoundShape->getChildShape(i);
                if (childShape->isConvex()) {
                    const btConvexShape* convexShape = static_cast<const btConvexShape*>(childShape);
                    copyShapeToMesh(compoundShape->getChildTransform(i), convexShape, mesh);
                }
            }
        } else if (shape->isConvex()) {
            const btConvexShape* convexShape = static_cast<const btConvexShape*>(shape);
            copyShapeToMesh(btTransform(), convexShape, mesh);
        }
    }
    return mesh;
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
        if ((*itr).second.use_count() == 1) {
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

