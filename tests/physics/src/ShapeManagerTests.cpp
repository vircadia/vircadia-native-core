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
#include <ShapeInfoUtil.h>
#include <ShapeManager.h>
#include <StreamUtils.h>

#include "ShapeManagerTests.h"

void ShapeManagerTests::testShapeAccounting() {
#ifdef USE_BULLET_PHYSICS
    ShapeManager shapeManager;
    ShapeInfo info;
    info.setBox(glm::vec3(1.0f, 1.0f, 1.0f));
    
    // NOTE: ShapeManager returns -1 as refcount when the shape is unknown, 
    // which is distinct from "known but with zero references"
    int numReferences = shapeManager.getNumReferences(info);
    if (numReferences != -1) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected ignorant ShapeManager after initialization" << std::endl;
    }

    // create one shape and verify we get a valid pointer
    btCollisionShape* shape = shapeManager.getShape(info);
    if (!shape) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected shape creation for default parameters" << std::endl;
    }

    // verify number of shapes
    if (shapeManager.getNumShapes() != 1) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: expected one shape" << std::endl;
    }

    // reference the shape again and verify that we get the same pointer
    btCollisionShape* otherShape = shapeManager.getShape(info);
    if (otherShape != shape) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: expected shape* " << (void*)(shape) 
            << " but found shape* " << (void*)(otherShape) << std::endl;
    }

    // verify number of references
    numReferences = shapeManager.getNumReferences(info);
    int expectedNumReferences = 2;
    if (numReferences != expectedNumReferences) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: expected " << expectedNumReferences 
            << " references but found " << numReferences << std::endl;
    }

    // release all references
    bool released = shapeManager.releaseShape(info);
    numReferences--;
    while (numReferences > 0) {
        released = shapeManager.releaseShape(info) && released;
        numReferences--;
    }
    if (!released) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: expected shape released" << std::endl;
    }

    // verify shape still exists (not yet garbage collected)
    if (shapeManager.getNumShapes() != 1) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: expected one shape after release but before garbage collection" << std::endl;
    }

    // verify shape's refcount is zero
    numReferences = shapeManager.getNumReferences(info);
    if (numReferences != 0) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected refcount = 0 for shape but found refcount = " << numReferences << std::endl;
    }

    // reference the shape again and verify refcount is updated
    otherShape = shapeManager.getShape(info);
    numReferences = shapeManager.getNumReferences(info);
    if (numReferences != 1) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected refcount = 1 for shape but found refcount = " << numReferences << std::endl;
    }

    // verify that shape is not collected as garbage
    shapeManager.collectGarbage();
    if (shapeManager.getNumShapes() != 1) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: expected one shape after release" << std::endl;
    }
    numReferences = shapeManager.getNumReferences(info);
    if (numReferences != 1) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected refcount = 1 for shape but found refcount = " << numReferences << std::endl;
    }

    // release reference and verify that it is collected as garbage
    released = shapeManager.releaseShape(info);
    shapeManager.collectGarbage();
    if (shapeManager.getNumShapes() != 0) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: expected zero shapes after release" << std::endl;
    }
    numReferences = shapeManager.getNumReferences(info);
    if (numReferences != -1) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected ignorant ShapeManager after garbage collection" << std::endl;
    }

    // add the shape again and verify that it gets added again
    otherShape = shapeManager.getShape(info);
    numReferences = shapeManager.getNumReferences(info);
    if (numReferences != 1) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected refcount = 1 for shape but found refcount = " << numReferences << std::endl;
    }
#endif // USE_BULLET_PHYSICS
}

void ShapeManagerTests::addManyShapes() {
#ifdef USE_BULLET_PHYSICS
    ShapeManager shapeManager;

    int numSizes = 100;
    float startSize = 1.0f;
    float endSize = 99.0f;
    float deltaSize = (endSize - startSize) / (float)numSizes;
    ShapeInfo info;
    for (int i = 0; i < numSizes; ++i) {
        float s = startSize + (float)i * deltaSize;
        glm::vec3 scale(s, 1.23f + s, s - 0.573f);
        info.setBox(0.5f * scale);
        btCollisionShape* shape = shapeManager.getShape(info);
        if (!shape) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: i = " << i << " null box shape for scale = " << scale << std::endl;
        }
        info.setSphere(s);
        shape = shapeManager.getShape(info);
        if (!shape) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: i = " << i << " null sphere shape for radius = " << s << std::endl;
        }
    }
    int numShapes = shapeManager.getNumShapes();
    if (numShapes != 2 * numSizes) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected numShapes = " << numSizes << " but found numShapes = " << numShapes << std::endl;
    }
#endif // USE_BULLET_PHYSICS
}

void ShapeManagerTests::addBoxShape() {
#ifdef USE_BULLET_PHYSICS
    ShapeInfo info;
    glm::vec3 halfExtents(1.23f, 4.56f, 7.89f);
    info.setBox(halfExtents);

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo;
    ShapeInfoUtil::collectInfoFromShape(shape, otherInfo);

    btCollisionShape* otherShape = shapeManager.getShape(otherInfo);
    if (shape != otherShape) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: Box ShapeInfo --> shape --> ShapeInfo --> shape did not work" << std::endl;
    }
#endif // USE_BULLET_PHYSICS
}

void ShapeManagerTests::addSphereShape() {
#ifdef USE_BULLET_PHYSICS
    ShapeInfo info;
    float radius = 1.23f;
    info.setSphere(radius);

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo;
    ShapeInfoUtil::collectInfoFromShape(shape, otherInfo);

    btCollisionShape* otherShape = shapeManager.getShape(otherInfo);
    if (shape != otherShape) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: Sphere ShapeInfo --> shape --> ShapeInfo --> shape did not work" << std::endl;
    }
#endif // USE_BULLET_PHYSICS
}

void ShapeManagerTests::addCylinderShape() {
#ifdef USE_BULLET_PHYSICS
    ShapeInfo info;
    float radius = 1.23f;
    float height = 4.56f;
    info.setCylinder(radius, height);

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo;
    ShapeInfoUtil::collectInfoFromShape(shape, otherInfo);

    btCollisionShape* otherShape = shapeManager.getShape(otherInfo);
    if (shape != otherShape) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: Cylinder ShapeInfo --> shape --> ShapeInfo --> shape did not work" << std::endl;
    }
#endif // USE_BULLET_PHYSICS
}

void ShapeManagerTests::addCapsuleShape() {
#ifdef USE_BULLET_PHYSICS
    ShapeInfo info;
    float radius = 1.23f;
    float height = 4.56f;
    info.setCapsule(radius, height);

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo;
    ShapeInfoUtil::collectInfoFromShape(shape, otherInfo);

    btCollisionShape* otherShape = shapeManager.getShape(otherInfo);
    if (shape != otherShape) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: Capsule ShapeInfo --> shape --> ShapeInfo --> shape did not work" << std::endl;
    }
#endif // USE_BULLET_PHYSICS
}

void ShapeManagerTests::runAllTests() {
    testShapeAccounting();
    addManyShapes();
    addBoxShape();
    addSphereShape();
    addCylinderShape();
    addCapsuleShape();
}
