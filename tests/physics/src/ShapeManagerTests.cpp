//
//  ShapeManagerTests.cpp
//  tests/physics/src
//
//  Created by Andrew Meadows on 2014.10.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <ShapeManager.h>
#include <StreamUtils.h>

#include "ShapeManagerTests.h"

QTEST_MAIN(ShapeManagerTests)

void ShapeManagerTests::testShapeAccounting() {
    ShapeManager shapeManager;
    ShapeInfo info;
    info.setBox(glm::vec3(1.0f, 1.0f, 1.0f));
    
    int numReferences = shapeManager.getNumReferences(info);
    QCOMPARE(numReferences, 0);

    // create one shape and verify we get a valid pointer
    btCollisionShape* shape = shapeManager.getShape(info);
    QCOMPARE(shape != nullptr, true);

    // verify number of shapes
    QCOMPARE(shapeManager.getNumShapes(), 1);

    // reference the shape again and verify that we get the same pointer
    btCollisionShape* otherShape = shapeManager.getShape(info);
    QCOMPARE(otherShape, shape);

    // verify number of references
    numReferences = shapeManager.getNumReferences(info);
    int expectedNumReferences = 2;
    QCOMPARE(numReferences, expectedNumReferences);

    // release all references
    bool released = shapeManager.releaseShape(info);
    numReferences--;
    while (numReferences > 0) {
        released = shapeManager.releaseShape(info) && released;
        numReferences--;
    }
    QCOMPARE(released, true);

    // verify shape still exists (not yet garbage collected)
    QCOMPARE(shapeManager.getNumShapes(), 1);

    // verify shape's refcount is zero
    numReferences = shapeManager.getNumReferences(info);
    QCOMPARE(numReferences, 0);

    // reference the shape again and verify refcount is updated
    otherShape = shapeManager.getShape(info);
    numReferences = shapeManager.getNumReferences(info);
    QCOMPARE(numReferences, 1);

    // verify that shape is not collected as garbage
    shapeManager.collectGarbage();
    QCOMPARE(shapeManager.getNumShapes(), 1);
    numReferences = shapeManager.getNumReferences(info);
    QCOMPARE(numReferences, 1);

    // release reference and verify that it is collected as garbage
    released = shapeManager.releaseShape(info);
    shapeManager.collectGarbage();
    QCOMPARE(shapeManager.getNumShapes(), 0);
    QCOMPARE(shapeManager.hasShape(shape), false);

    // add the shape again and verify that it gets added again
    otherShape = shapeManager.getShape(info);
    numReferences = shapeManager.getNumReferences(info);
    QCOMPARE(numReferences, 1);
}

void ShapeManagerTests::addManyShapes() {
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

void ShapeManagerTests::addBoxShape() {
    ShapeInfo info;
    glm::vec3 halfExtents(1.23f, 4.56f, 7.89f);
    info.setBox(halfExtents);

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo = info;
    btCollisionShape* otherShape = shapeManager.getShape(otherInfo);
    QCOMPARE(shape, otherShape);
}

void ShapeManagerTests::addSphereShape() {
    ShapeInfo info;
    float radius = 1.23f;
    info.setSphere(radius);

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo = info;
    btCollisionShape* otherShape = shapeManager.getShape(otherInfo);
    QCOMPARE(shape, otherShape);
}

void ShapeManagerTests::addCylinderShape() {
    /* TODO: reimplement Cylinder shape
    ShapeInfo info;
    float radius = 1.23f;
    float height = 4.56f;
    info.setCylinder(radius, height);

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo = info;
    btCollisionShape* otherShape = shapeManager.getShape(otherInfo);
    QCOMPARE(shape, otherShape);
    */
}

void ShapeManagerTests::addCapsuleShape() {
    /* TODO: reimplement Capsule shape
    ShapeInfo info;
    float radius = 1.23f;
    float height = 4.56f;
    info.setCapsule(radius, height);

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo = info;
    btCollisionShape* otherShape = shapeManager.getShape(otherInfo);
    QCOMPARE(shape, otherShape);
    */
}
