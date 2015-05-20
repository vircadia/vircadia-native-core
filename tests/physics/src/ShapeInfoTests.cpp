//
//  ShapeInfoTests.cpp
//  tests/physics/src
//
//  Created by Andrew Meadows on 2014.11.02
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>

#include <btBulletDynamicsCommon.h>
#include <LinearMath/btHashMap.h>

#include <DoubleHashKey.h>
#include <ShapeInfo.h>
#include <ShapeInfoUtil.h>
#include <StreamUtils.h>

#include "ShapeInfoTests.h"

void ShapeInfoTests::testHashFunctions() {
    int maxTests = 10000000;
    ShapeInfo info;
    btHashMap<btHashInt, uint32_t> hashes;

    uint32_t bits[32];
    uint32_t masks[32];
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
        DoubleHashKey key = info.getHash();
        uint32_t* hashPtr = hashes.find(key.getHash());
        if (hashPtr && *hashPtr == key.getHash2()) {
            std::cout << testCount << "  hash collision radiusX = " << radiusX 
                << "  h1 = 0x" << std::hex << key.getHash()
                << "  h2 = 0x" << std::hex << key.getHash2()
                << std::endl;
            ++numCollisions;
            assert(false);
        } else {
            hashes.insert(key.getHash(), key.getHash2());
        }
        for (int k = 0; k < 32; ++k) {
            if (masks[k] & key.getHash2()) {
                ++bits[k];
            }
        }

        for (int y = 1; y < numSteps && testCount < maxTests; ++y) {
            float radiusY = (float)y * deltaLength;
            /* TODO: reimplement Cylinder and Capsule shapes
            // test cylinder and capsule
            int types[] = { CYLINDER_SHAPE_PROXYTYPE, CAPSULE_SHAPE_PROXYTYPE };
            for (int i = 0; i < 2; ++i) {
                switch(types[i]) {
                    case CYLINDER_SHAPE_PROXYTYPE: {
                        info.setCylinder(radiusX, radiusY);
                        break;
                    }
                    case CAPSULE_SHAPE_PROXYTYPE: {
                        info.setCapsuleY(radiusX, radiusY);
                        break;
                    }
                }

                ++testCount;
                key = info.getHash();
                hashPtr = hashes.find(key.getHash());
                if (hashPtr && *hashPtr == key.getHash2()) {
                    std::cout << testCount << "  hash collision radiusX = " << radiusX << "  radiusY = " << radiusY 
                        << "  h1 = 0x" << std::hex << key.getHash()
                        << "  h2 = 0x" << std::hex << key.getHash2()
                        << std::endl;
                    ++numCollisions;
                    assert(false);
                } else {
                    hashes.insert(key.getHash(), key.getHash2());
                }
                for (int k = 0; k < 32; ++k) {
                    if (masks[k] & key.getHash2()) {
                        ++bits[k];
                    }
                }
            }
            */

            for (int z = 1; z < numSteps && testCount < maxTests; ++z) {
                float radiusZ = (float)z * deltaLength;
                // test box
                info.setBox(glm::vec3(radiusX, radiusY, radiusZ));
                ++testCount;
                DoubleHashKey key = info.getHash();
                hashPtr = hashes.find(key.getHash());
                if (hashPtr && *hashPtr == key.getHash2()) {
                    std::cout << testCount << "  hash collision radiusX = " << radiusX 
                        << "  radiusY = " << radiusY << "  radiusZ = " << radiusZ 
                        << "  h1 = 0x" << std::hex << key.getHash()
                        << "  h2 = 0x" << std::hex << key.getHash2()
                        << std::endl;
                    ++numCollisions;
                    assert(false);
                } else {
                    hashes.insert(key.getHash(), key.getHash2());
                }
                for (int k = 0; k < 32; ++k) {
                    if (masks[k] & key.getHash2()) {
                        ++bits[k];
                    }
                }
            }
        }
    }
    uint64_t msec = timer.getTimeMilliseconds();
    std::cout << msec << " msec with " << numCollisions << " collisions out of " << testCount << " hashes" << std::endl;

    // print out distribution of bits
    for (int i = 0; i < 32; ++i) {
        std::cout << "bit 0x" << std::hex << masks[i] << std::dec << " = " << bits[i] << std::endl;
    }
}

void ShapeInfoTests::testBoxShape() {
    ShapeInfo info;
    glm::vec3 halfExtents(1.23f, 4.56f, 7.89f);
    info.setBox(halfExtents);
    DoubleHashKey key = info.getHash();

    btCollisionShape* shape = ShapeInfoUtil::createShapeFromInfo(info);
    if (!shape) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: NULL Box shape" << std::endl;
    }

    ShapeInfo otherInfo = info;
    DoubleHashKey otherKey = otherInfo.getHash();
    if (key.getHash() != otherKey.getHash()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Box shape hash = " << key.getHash() << " but found hash = " << otherKey.getHash() << std::endl;
    }

    if (key.getHash2() != otherKey.getHash2()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Box shape hash2 = " << key.getHash2() << " but found hash2 = " << otherKey.getHash2() << std::endl;
    }

    delete shape;
}

void ShapeInfoTests::testSphereShape() {
    ShapeInfo info;
    float radius = 1.23f;
    info.setSphere(radius);
    DoubleHashKey key = info.getHash();

    btCollisionShape* shape = ShapeInfoUtil::createShapeFromInfo(info);

    ShapeInfo otherInfo = info;
    DoubleHashKey otherKey = otherInfo.getHash();
    if (key.getHash() != otherKey.getHash()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Sphere shape hash = " << key.getHash() << " but found hash = " << otherKey.getHash() << std::endl;
    }
    if (key.getHash2() != otherKey.getHash2()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Sphere shape hash2 = " << key.getHash2() << " but found hash2 = " << otherKey.getHash2() << std::endl;
    }

    delete shape;
}

void ShapeInfoTests::testCylinderShape() {
    /* TODO: reimplement Cylinder shape
    ShapeInfo info;
    float radius = 1.23f;
    float height = 4.56f;
    info.setCylinder(radius, height);
    DoubleHashKey key = info.getHash();

    btCollisionShape* shape = ShapeInfoUtil::createShapeFromInfo(info);

    ShapeInfo otherInfo = info;
    DoubleHashKey otherKey = otherInfo.getHash();
    if (key.getHash() != otherKey.getHash()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Cylinder shape hash = " << key.getHash() << " but found hash = " << otherKey.getHash() << std::endl;
    }
    if (key.getHash2() != otherKey.getHash2()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Cylinder shape hash2 = " << key.getHash2() << " but found hash2 = " << otherKey.getHash2() << std::endl;
    }

    delete shape;
    */
}

void ShapeInfoTests::testCapsuleShape() {
    /* TODO: reimplement Capsule shape
    ShapeInfo info;
    float radius = 1.23f;
    float height = 4.56f;
    info.setCapsule(radius, height);
    DoubleHashKey key = info.getHash();

    btCollisionShape* shape = ShapeInfoUtil::createShapeFromInfo(info);

    ShapeInfo otherInfo = info;
    DoubleHashKey otherKey = otherInfo.getHash();
    if (key.getHash() != otherKey.getHash()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Capsule shape hash = " << key.getHash() << " but found hash = " << otherKey.getHash() << std::endl;
    }
    if (key.getHash2() != otherKey.getHash2()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Capsule shape hash2 = " << key.getHash2() << " but found hash2 = " << otherKey.getHash2() << std::endl;
    }

    delete shape;
    */
}

void ShapeInfoTests::runAllTests() {
//#define MANUAL_TEST
#ifdef MANUAL_TEST
    testHashFunctions();
#endif // MANUAL_TEST
    testBoxShape();
    testSphereShape();
    testCylinderShape();
    testCapsuleShape();
}
