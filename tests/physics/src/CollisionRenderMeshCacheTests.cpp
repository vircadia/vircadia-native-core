//
//  CollisionRenderMeshCacheTests.cpp
//  tests/physics/src
//
//  Created by Andrew Meadows on 2014.10.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CollisionRenderMeshCacheTests.h"

#include <iostream>
#include <cstdlib>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>

#include <CollisionRenderMeshCache.h>
#include <ShapeInfo.h> // for MAX_HULL_POINTS

#include "MeshUtil.cpp"


QTEST_MAIN(CollisionRenderMeshCacheTests)

const float INV_SQRT_THREE = 0.577350269f;

const uint32_t numSphereDirections = 6 + 8;
btVector3 sphereDirections[] = {
    btVector3(1.0f, 0.0f, 0.0f),
    btVector3(-1.0f, 0.0f, 0.0f),
    btVector3(0.0f, 1.0f, 0.0f),
    btVector3(0.0f, -1.0f, 0.0f),
    btVector3(0.0f, 0.0f, 1.0f),
    btVector3(0.0f, 0.0f, -1.0f),
    btVector3(INV_SQRT_THREE, INV_SQRT_THREE, INV_SQRT_THREE),
    btVector3(INV_SQRT_THREE, INV_SQRT_THREE, -INV_SQRT_THREE),
    btVector3(INV_SQRT_THREE, -INV_SQRT_THREE, INV_SQRT_THREE),
    btVector3(INV_SQRT_THREE, -INV_SQRT_THREE, -INV_SQRT_THREE),
    btVector3(-INV_SQRT_THREE, INV_SQRT_THREE, INV_SQRT_THREE),
    btVector3(-INV_SQRT_THREE, INV_SQRT_THREE, -INV_SQRT_THREE),
    btVector3(-INV_SQRT_THREE, -INV_SQRT_THREE, INV_SQRT_THREE),
    btVector3(-INV_SQRT_THREE, -INV_SQRT_THREE, -INV_SQRT_THREE)
};

float randomFloat() {
    return 2.0f * ((float)rand() / (float)RAND_MAX) - 1.0f;
}

btBoxShape* createBoxShape(const btVector3& extent) {
    btBoxShape* shape = new btBoxShape(0.5f * extent);
    return shape;
}

btConvexHullShape* createConvexHull(float radius) {
    btConvexHullShape* hull = new btConvexHullShape();
    for (uint32_t i = 0; i < numSphereDirections; ++i) {
        btVector3 point = radius * sphereDirections[i];
        hull->addPoint(point, false);
    }
    hull->recalcLocalAabb();
    return hull;
}

void CollisionRenderMeshCacheTests::testShapeHullManifold() {
    // make a box shape
    btVector3 extent(1.0f, 2.0f, 3.0f);
    btBoxShape* box = createBoxShape(extent);

    // wrap it with a ShapeHull
    btShapeHull hull(box);
    const float MARGIN = 0.0f;
    hull.buildHull(MARGIN);

    // verify the vertex count is capped
    uint32_t numVertices = (uint32_t)hull.numVertices();
    QVERIFY(numVertices <= MAX_HULL_POINTS);

    // verify the mesh is inside the radius
    btVector3 halfExtents = box->getHalfExtentsWithMargin();
    float ACCEPTABLE_EXTENTS_ERROR = 0.01f;
    float maxRadius = halfExtents.length() + ACCEPTABLE_EXTENTS_ERROR;
    const btVector3* meshVertices = hull.getVertexPointer();
    for (uint32_t i = 0; i < numVertices; ++i) {
        btVector3 vertex = meshVertices[i];
        QVERIFY(vertex.length() <= maxRadius);
    }

    // verify the index count is capped
    uint32_t numIndices = (uint32_t)hull.numIndices();
    QVERIFY(numIndices < 6 * MAX_HULL_POINTS);

    // verify the index count is a multiple of 3
    QVERIFY(numIndices % 3 == 0);

    // verify the mesh is closed
    const uint32_t* meshIndices = hull.getIndexPointer();
    bool isClosed = MeshUtil::isClosedManifold(meshIndices, numIndices);
    QVERIFY(isClosed);

    // verify the triangle normals are outward using right-hand-rule
    const uint32_t INDICES_PER_TRIANGLE = 3;
    for (uint32_t i = 0; i < numIndices; i += INDICES_PER_TRIANGLE) {
        btVector3 A = meshVertices[meshIndices[i]];
        btVector3 B = meshVertices[meshIndices[i+1]];
        btVector3 C = meshVertices[meshIndices[i+2]];

        btVector3 face = (B - A).cross(C - B);
        btVector3 center = (A + B + C) / 3.0f;
        QVERIFY(face.dot(center) > 0.0f);
    }

    // delete unmanaged memory
    delete box;
}

void CollisionRenderMeshCacheTests::testCompoundShape() {
    uint32_t numSubShapes = 3;

    btVector3 centers[] = {
        btVector3(1.0f, 0.0f, 0.0f),
        btVector3(0.0f, -2.0f, 0.0f),
        btVector3(0.0f, 0.0f, 3.0f),
    };

    float radii[] = { 3.0f, 2.0f, 1.0f };

    btCompoundShape* compoundShape = new btCompoundShape();
    for (uint32_t i = 0; i < numSubShapes; ++i) {
        btTransform transform;
        transform.setOrigin(centers[i]);
        btConvexHullShape* hull = createConvexHull(radii[i]);
        compoundShape->addChildShape(transform, hull);
    }

    // create the cache
    CollisionRenderMeshCache cache;
    QVERIFY(cache.getNumMeshes() == 0);

    // get the mesh once
    model::MeshPointer mesh = cache.getMesh(compoundShape);
    QVERIFY((bool)mesh);
    QVERIFY(cache.getNumMeshes() == 1);

    // get the mesh again
    model::MeshPointer mesh2 = cache.getMesh(compoundShape);
    QVERIFY(mesh2 == mesh);
    QVERIFY(cache.getNumMeshes() == 1);

    // forget the mesh once
    cache.releaseMesh(compoundShape);
    mesh.reset();
    QVERIFY(cache.getNumMeshes() == 1);

    // collect garbage (should still cache mesh)
    cache.collectGarbage();
    QVERIFY(cache.getNumMeshes() == 1);

    // forget the mesh a second time (should still cache mesh)
    cache.releaseMesh(compoundShape);
    mesh2.reset();
    QVERIFY(cache.getNumMeshes() == 1);

    // collect garbage (should no longer cache mesh)
    cache.collectGarbage();
    QVERIFY(cache.getNumMeshes() == 0);

    // delete unmanaged memory
    for (int i = 0; i < compoundShape->getNumChildShapes(); ++i) {
        delete compoundShape->getChildShape(i);
    }
    delete compoundShape;
}

void CollisionRenderMeshCacheTests::testMultipleShapes() {
    // shapeA is compound of hulls
    uint32_t numSubShapes = 3;
    btVector3 centers[] = {
        btVector3(1.0f, 0.0f, 0.0f),
        btVector3(0.0f, -2.0f, 0.0f),
        btVector3(0.0f, 0.0f, 3.0f),
    };
    float radii[] = { 3.0f, 2.0f, 1.0f };
    btCompoundShape* shapeA = new btCompoundShape();
    for (uint32_t i = 0; i < numSubShapes; ++i) {
        btTransform transform;
        transform.setOrigin(centers[i]);
        btConvexHullShape* hull = createConvexHull(radii[i]);
        shapeA->addChildShape(transform, hull);
    }

    // shapeB is compound of boxes
    btVector3 extents[] = {
        btVector3(1.0f, 2.0f, 3.0f),
        btVector3(2.0f, 3.0f, 1.0f),
        btVector3(3.0f, 1.0f, 2.0f),
    };
    btCompoundShape* shapeB = new btCompoundShape();
    for (uint32_t i = 0; i < numSubShapes; ++i) {
        btTransform transform;
        transform.setOrigin(centers[i]);
        btBoxShape* box = createBoxShape(extents[i]);
        shapeB->addChildShape(transform, box);
    }

    // shapeC is just a box
    btVector3 extentC(7.0f, 3.0f, 5.0f);
    btBoxShape* shapeC = createBoxShape(extentC);

    // create the cache
    CollisionRenderMeshCache cache;
    QVERIFY(cache.getNumMeshes() == 0);

    // get the meshes
    model::MeshPointer meshA = cache.getMesh(shapeA);
    model::MeshPointer meshB = cache.getMesh(shapeB);
    model::MeshPointer meshC = cache.getMesh(shapeC);
    QVERIFY((bool)meshA);
    QVERIFY((bool)meshB);
    QVERIFY((bool)meshC);
    QVERIFY(cache.getNumMeshes() == 3);

    // get the meshes again
    model::MeshPointer meshA2 = cache.getMesh(shapeA);
    model::MeshPointer meshB2 = cache.getMesh(shapeB);
    model::MeshPointer meshC2 = cache.getMesh(shapeC);
    QVERIFY(meshA == meshA2);
    QVERIFY(meshB == meshB2);
    QVERIFY(meshC == meshC2);
    QVERIFY(cache.getNumMeshes() == 3);

    // forget the meshes once
    cache.releaseMesh(shapeA);
    cache.releaseMesh(shapeB);
    cache.releaseMesh(shapeC);
    meshA2.reset();
    meshB2.reset();
    meshC2.reset();
    QVERIFY(cache.getNumMeshes() == 3);

    // collect garbage (should still cache mesh)
    cache.collectGarbage();
    QVERIFY(cache.getNumMeshes() == 3);

    // forget again, one mesh at a time...
    // shapeA...
    cache.releaseMesh(shapeA);
    meshA.reset();
    QVERIFY(cache.getNumMeshes() == 3);
    cache.collectGarbage();
    QVERIFY(cache.getNumMeshes() == 2);
    // shapeB...
    cache.releaseMesh(shapeB);
    meshB.reset();
    QVERIFY(cache.getNumMeshes() == 2);
    cache.collectGarbage();
    QVERIFY(cache.getNumMeshes() == 1);
    // shapeC...
    cache.releaseMesh(shapeC);
    meshC.reset();
    QVERIFY(cache.getNumMeshes() == 1);
    cache.collectGarbage();
    QVERIFY(cache.getNumMeshes() == 0);

    // delete unmanaged memory
    for (int i = 0; i < shapeA->getNumChildShapes(); ++i) {
        delete shapeA->getChildShape(i);
    }
    delete shapeA;
    for (int i = 0; i < shapeB->getNumChildShapes(); ++i) {
        delete shapeB->getChildShape(i);
    }
    delete shapeB;
    delete shapeC;
}
