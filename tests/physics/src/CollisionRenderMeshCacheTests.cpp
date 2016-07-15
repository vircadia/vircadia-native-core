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

btVector3 directions[] = {
    btVector3(1.0f, 1.0f, 1.0f),
    btVector3(1.0f, 1.0f, -1.0f),
    btVector3(1.0f, -1.0f, 1.0f),
    btVector3(1.0f, -1.0f, -1.0f),
    btVector3(-1.0f, 1.0f, 1.0f),
    btVector3(-1.0f, 1.0f, -1.0f),
    btVector3(-1.0f, -1.0f, 1.0f),
    btVector3(-1.0f, -1.0f, -1.0f)
};

void computeCubePoints(const btVector3& center, btScalar radius, uint32_t numPoints,
        btAlignedObjectArray<btVector3>& points) {
    points.reserve(points.size() + 8);
    for (uint32_t i = 0; i < 8; ++i) {
        points.push_back(center + radius * directions[i]);
    }
}

float randomFloat() {
    return (float)rand() / (float)RAND_MAX;
}

btBoxShape* createRandomCubeShape() {
    //const btScalar MAX_RADIUS = 3.0;
    //const btScalar MIN_RADIUS = 0.5;
    //btScalar radius = randomFloat() * (MAX_RADIUS - MIN_RADIUS) + MIN_RADIUS;
    btScalar radius = 0.5f;
    btVector3 halfExtents(radius, radius, radius);

    btBoxShape* shape = new btBoxShape(halfExtents);
    return shape;
}

void CollisionRenderMeshCacheTests::test001() {
    // make a box shape
    btBoxShape* box = createRandomCubeShape();

    // wrap it with a ShapeHull
    btShapeHull hull(box);
    //const btScalar MARGIN = 0.01f;
    const btScalar MARGIN = 0.00f;
    hull.buildHull(MARGIN);

    // verify the vertex count is capped
    uint32_t numVertices = (uint32_t)hull.numVertices();
    QVERIFY(numVertices <= MAX_HULL_POINTS);

    // verify the mesh is inside the radius
    btVector3 halfExtents = box->getHalfExtentsWithMargin();
    btScalar acceptableRadiusError = 0.01f;
    btScalar maxRadius = halfExtents.length() + acceptableRadiusError;
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

    delete box;
}

#ifdef FOO
void CollisionRenderMeshCacheTests::test001() {
    CollisionRenderMeshCache cache;

    // create a compound shape
    btScalar radiusA = 1.0f;
    btScalar radiusB = 1.5f;
    btScalar radiusC = 2.75f;

    btVector3 centerA(radiusA, 0.0f, 0.0f);
    btVector3 centerB(0.0f, radiusB, 0.0f);
    btVector3 centerC(0.0f, 0.0f, radiusC);

    btCompoundShape compoundShape = new btCompoundShape();
    for (uint32_t i = 0; i < numSubShapes; ++i) {
    }


    // get the mesh

    // get the mesh again

    // forget the mesh once

    // collect garbage

    // forget the mesh a second time

    // collect garbage

}
#endif // FOO

/*
void CollisionRenderMeshCacheTests::addManyShapes() {
    ShapeManager shapeManager;

    QVector<btCollisionShape*> shapes;

    int numSizes = 100;
    float startSize = 1.0f;
    float endSize = 99.0f;
    float deltaSize = (endSize - startSize) / (float)numSizes;
    ShapeInfo info;
    for (int i = 0; i < numSizes; ++i) {
        // make a sphere
        float s = startSize + (float)i * deltaSize;
        glm::vec3 scale(s, 1.23f + s, s - 0.573f);
        info.setBox(0.5f * scale);
        btCollisionShape* shape = shapeManager.getShape(info);
        shapes.push_back(shape);
        QCOMPARE(shape != nullptr, true);

        // make a box
        float radius = 0.5f * s;
        info.setSphere(radius);
        shape = shapeManager.getShape(info);
        shapes.push_back(shape);
        QCOMPARE(shape != nullptr, true);
    }

    // verify shape count
    int numShapes = shapeManager.getNumShapes();
    QCOMPARE(numShapes, 2 * numSizes);

    // release each shape by pointer
    for (int i = 0; i < numShapes; ++i) {
        btCollisionShape* shape = shapes[i];
        bool success = shapeManager.releaseShape(shape);
        QCOMPARE(success, true);
    }

    // verify zero references
    for (int i = 0; i < numShapes; ++i) {
        btCollisionShape* shape = shapes[i];
        int numReferences = shapeManager.getNumReferences(shape);
        QCOMPARE(numReferences, 0);
    }
}

void CollisionRenderMeshCacheTests::addBoxShape() {
    ShapeInfo info;
    glm::vec3 halfExtents(1.23f, 4.56f, 7.89f);
    info.setBox(halfExtents);

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo = info;
    btCollisionShape* otherShape = shapeManager.getShape(otherInfo);
    QCOMPARE(shape, otherShape);
}

void CollisionRenderMeshCacheTests::addSphereShape() {
    ShapeInfo info;
    float radius = 1.23f;
    info.setSphere(radius);

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo = info;
    btCollisionShape* otherShape = shapeManager.getShape(otherInfo);
    QCOMPARE(shape, otherShape);
}

void CollisionRenderMeshCacheTests::addCompoundShape() {
    // initialize some points for generating tetrahedral convex hulls
    QVector<glm::vec3> tetrahedron;
    tetrahedron.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
    tetrahedron.push_back(glm::vec3(1.0f, -1.0f, -1.0f));
    tetrahedron.push_back(glm::vec3(-1.0f, 1.0f, -1.0f));
    tetrahedron.push_back(glm::vec3(-1.0f, -1.0f, 1.0f));
    int numHullPoints = tetrahedron.size();

    // compute the points of the hulls
    ShapeInfo::PointCollection pointCollection;
    int numHulls = 5;
    glm::vec3 offsetNormal(1.0f, 0.0f, 0.0f);
    for (int i = 0; i < numHulls; ++i) {
        glm::vec3 offset = (float)(i - numHulls/2) * offsetNormal;
        ShapeInfo::PointList pointList;
        float radius = (float)(i + 1);
        for (int j = 0; j < numHullPoints; ++j) {
            glm::vec3 point = radius * tetrahedron[j] + offset;
            pointList.push_back(point);
        }
        pointCollection.push_back(pointList);
    }

    // create the ShapeInfo
    ShapeInfo info;
    info.setPointCollection(hulls);

    // create the shape
    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);
    QVERIFY(shape != nullptr);

    // verify the shape is correct type
    QCOMPARE(shape->getShapeType(), (int)COMPOUND_SHAPE_PROXYTYPE);

    // verify the shape has correct number of children
    btCompoundShape* compoundShape = static_cast<btCompoundShape*>(shape);
    QCOMPARE(compoundShape->getNumChildShapes(), numHulls);

    // verify manager has only one shape
    QCOMPARE(shapeManager.getNumShapes(), 1);
    QCOMPARE(shapeManager.getNumReferences(info), 1);

    // release the shape
    shapeManager.releaseShape(shape);
    QCOMPARE(shapeManager.getNumShapes(), 1);
    QCOMPARE(shapeManager.getNumReferences(info), 0);

    // collect garbage
    shapeManager.collectGarbage();
    QCOMPARE(shapeManager.getNumShapes(), 0);
    QCOMPARE(shapeManager.getNumReferences(info), 0);
}
*/
