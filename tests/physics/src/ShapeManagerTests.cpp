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

void ShapeManagerTests::testHashFunctions() {
#ifdef USE_BULLET_PHYSICS
    int maxTests = 10000000;
    ShapeInfo info;
    btHashMap<btHashInt, int> hashes;

    int bits[32];
    unsigned int masks[32];
    for (int i = 0; i < 32; ++i) {
        bits[i] = 0;
        masks[i] = 1U << i;
    }

    float deltaLength = 0.002f;
    float endLength = 100.0f;
    int numSteps = (int)(endLength / deltaLength);

    int testCount = 0;
    int numCollisions = 0;
    btClock timer;
    for (int x = 1; x < numSteps && testCount < maxTests; ++x) {
        float radiusX = (float)x * deltaLength;
        // test sphere
        info.setSphere(radiusX);
        ++testCount;
        int hash = info.computeHash();
        int hash2 = info.computeHash2();
        int* hashPtr = hashes.find(hash);
        if (hashPtr && *hashPtr == hash2) {
            std::cout << testCount << "  hash collision radiusX = " << radiusX 
                << "  h1 = 0x" << std::hex << (unsigned int)(hash) 
                << "  h2 = 0x" << std::hex << (unsigned int)(hash2) 
                << std::endl;
            ++numCollisions;
            assert(false);
        } else {
            hashes.insert(hash, hash2);
        }
        for (int k = 0; k < 32; ++k) {
            if (masks[k] & hash2) {
                ++bits[k];
            }
        }

        for (int y = 1; y < numSteps && testCount < maxTests; ++y) {
            float radiusY = (float)y * deltaLength;
            // test cylinder and capsule
            int types[] = { CYLINDER_SHAPE_PROXYTYPE, CAPSULE_SHAPE_PROXYTYPE };
            for (int i = 0; i < 2; ++i) {
                switch(types[i]) {
                    case CYLINDER_SHAPE_PROXYTYPE: {
                        info.setCylinder(radiusX, radiusY);
                        break;
                    }
                    case CAPSULE_SHAPE_PROXYTYPE: {
                        info.setCapsule(radiusX, radiusY);
                        break;
                    }
                }

                ++testCount;
                hash = info.computeHash();
                hash2 = info.computeHash2();
                hashPtr = hashes.find(hash);
                if (hashPtr && *hashPtr == hash2) {
                    std::cout << testCount << "  hash collision radiusX = " << radiusX << "  radiusY = " << radiusY 
                        << "  h1 = 0x" << std::hex << (unsigned int)(hash) 
                        << "  h2 = 0x" << std::hex << (unsigned int)(hash2) 
                        << std::endl;
                    ++numCollisions;
                    assert(false);
                } else {
                    hashes.insert(hash, hash2);
                }
                for (int k = 0; k < 32; ++k) {
                    if (masks[k] & hash2) {
                        ++bits[k];
                    }
                }
            }

            for (int z = 1; z < numSteps && testCount < maxTests; ++z) {
                float radiusZ = (float)z * deltaLength;
                // test box
                info.setBox(btVector3(radiusX, radiusY, radiusZ));
                ++testCount;
                hash = info.computeHash();
                hash2 = info.computeHash2();
                hashPtr = hashes.find(hash);
                if (hashPtr && *hashPtr == hash2) {
                    std::cout << testCount << "  hash collision radiusX = " << radiusX 
                        << "  radiusY = " << radiusY << "  radiusZ = " << radiusZ 
                        << "  h1 = 0x" << std::hex << (unsigned int)(hash) 
                        << "  h2 = 0x" << std::hex << (unsigned int)(hash2) 
                        << std::endl;
                    ++numCollisions;
                    assert(false);
                } else {
                    hashes.insert(hash, hash2);
                }
                for (int k = 0; k < 32; ++k) {
                    if (masks[k] & hash2) {
                        ++bits[k];
                    }
                }
            }
        }
    }
    unsigned long int msec = timer.getTimeMilliseconds();
    std::cout << msec << " msec with " << numCollisions << " collisions out of " << testCount << " hashes" << std::endl;

    // print out distribution of bits
    for (int i = 0; i < 32; ++i) {
        std::cout << "bit 0x" << std::hex << masks[i] << std::dec << " = " << bits[i] << std::endl;
    }
#endif // USE_BULLET_PHYSICS
}

void ShapeManagerTests::testShapeAccounting() {
#ifdef USE_BULLET_PHYSICS
    ShapeManager shapeManager;
    ShapeInfo info;
    info.setBox(btVector3(1.0f, 1.0f, 1.0f));
    
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
        btVector3 scale(s, 1.23f + s, s - 0.573f);
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

void ShapeManagerTests::testBoxShape() {
#ifdef USE_BULLET_PHYSICS
    ShapeInfo info;
    btVector3 halfExtents(1.23f, 4.56f, 7.89f);
    info.setBox(halfExtents);
    int hash = info.computeHash();

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo;
    otherInfo.collectInfo(shape);
    int otherHash = otherInfo.computeHash();

    if (hash != otherHash) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Box shape hash = " << hash << " but found hash = " << otherHash << std::endl;
    }

    btCollisionShape* otherShape = shapeManager.getShape(info);
    if (shape != otherShape) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: Box ShapeInfo --> shape --> ShapeInfo --> shape did not work" << std::endl;
    }
#endif // USE_BULLET_PHYSICS
}

void ShapeManagerTests::testSphereShape() {
#ifdef USE_BULLET_PHYSICS
    ShapeInfo info;
    float radius = 1.23f;
    info.setSphere(radius);
    int hash = info.computeHash();

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo;
    otherInfo.collectInfo(shape);
    int otherHash = otherInfo.computeHash();

    if (hash != otherHash) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Sphere shape hash = " << hash << " but found hash = " << otherHash << std::endl;
    }

    btCollisionShape* otherShape = shapeManager.getShape(info);
    if (shape != otherShape) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: Sphere ShapeInfo --> shape --> ShapeInfo --> shape did not work" << std::endl;
    }
#endif // USE_BULLET_PHYSICS
}

void ShapeManagerTests::testCylinderShape() {
#ifdef USE_BULLET_PHYSICS
    ShapeInfo info;
    float radius = 1.23f;
    float height = 4.56f;
    info.setCylinder(radius, height);
    int hash = info.computeHash();

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo;
    otherInfo.collectInfo(shape);
    int otherHash = otherInfo.computeHash();

    if (hash != otherHash) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Cylinder shape hash = " << hash << " but found hash = " << otherHash << std::endl;
    }

    btCollisionShape* otherShape = shapeManager.getShape(info);
    if (shape != otherShape) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: Cylinder ShapeInfo --> shape --> ShapeInfo --> shape did not work" << std::endl;
    }
#endif // USE_BULLET_PHYSICS
}

void ShapeManagerTests::testCapsuleShape() {
#ifdef USE_BULLET_PHYSICS
    ShapeInfo info;
    float radius = 1.23f;
    float height = 4.56f;
    info.setCapsule(radius, height);
    int hash = info.computeHash();

    ShapeManager shapeManager;
    btCollisionShape* shape = shapeManager.getShape(info);

    ShapeInfo otherInfo;
    otherInfo.collectInfo(shape);
    int otherHash = otherInfo.computeHash();

    if (hash != otherHash) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Capsule shape hash = " << hash << " but found hash = " << otherHash << std::endl;
    }

    btCollisionShape* otherShape = shapeManager.getShape(info);
    if (shape != otherShape) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: Capsule ShapeInfo --> shape --> ShapeInfo --> shape did not work" << std::endl;
    }
#endif // USE_BULLET_PHYSICS
}

void ShapeManagerTests::runAllTests() {
//#define MANUAL_TEST
#ifdef MANUAL_TEST
    testHashFunctions();
#endif // MANUAL_TEST
    testShapeAccounting();
    addManyShapes();
    testBoxShape();
    testSphereShape();
    testCylinderShape();
    testCapsuleShape();
}
