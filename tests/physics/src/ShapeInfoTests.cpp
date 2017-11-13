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

#include <HashKey.h>
#include <ShapeInfo.h>
#include <ShapeFactory.h>
#include <StreamUtils.h>

#include "ShapeInfoTests.h"

QTEST_MAIN(ShapeInfoTests)

// Enable this to manually run testHashCollisions
// (NOT a regular unit test; takes ~40 secs to run on an i7)
//#define MANUAL_TEST true

void ShapeInfoTests::testHashFunctions() {
#if MANUAL_TEST
    int maxTests = 10000000;
    ShapeInfo info;
    btHashMap<HashKey, int32_t> hashes;

    const int32_t NUM_HASH_BITS = 32;
    uint32_t bits[NUM_HASH_BITS];
    uint32_t masks[NUM_HASH_BITS];
    for (int i = 0; i < NUM_HASH_BITS; ++i) {
        bits[i] = 0;
        masks[i] = 1UL << i;
    }

    float deltaLength = 1.0f / (HashKey::getNumQuantizedValuesPerMeter() - 3.0f);
    float endLength = 2000.0f * deltaLength;
    int numSteps = (int)(endLength / deltaLength);

    int testCount = 0;
    int numCollisions = 0;

    btClock timer;
    for (int i = 1; i < numSteps && testCount < maxTests; ++i) {
        float radiusX = (float)i * deltaLength;
        int32_t* hashPtr;
        // test sphere
        info.setSphere(radiusX);
        ++testCount;
        HashKey key = info.getHash();
        hashPtr = hashes.find(key);
        if (hashPtr) {
            std::cout << testCount << "  hash collision sphere radius = " << radiusX
                << "  h = 0x" << std::hex << key.getHash() << " : 0x" << *hashPtr
                << std::dec << std::endl;
            ++numCollisions;
            assert(false);
        } else {
            hashes.insert(key, key.getHash());
        }
        // track bit distribution counts to evaluate hash function randomness
        for (int j = 0; j < NUM_HASH_BITS; ++j) {
            if (masks[j] & key.getHash()) {
                ++bits[j];
            }
        }

        for (int y = 1; y < numSteps && testCount < maxTests; ++y) {
            float radiusY = (float)y * deltaLength;
            for (int z = 1; z < numSteps && testCount < maxTests; ++z) {
                float radiusZ = (float)z * deltaLength;
                // test box
                info.setBox(glm::vec3(radiusX, radiusY, radiusZ));
                ++testCount;
                HashKey key = info.getHash();
                hashPtr = hashes.find(key);
                if (hashPtr) {
                    std::cout << testCount << "  hash collision box dimensions = < " << radiusX
                        << ", " << radiusY << ", " << radiusZ << " >"
                        << "  h = 0x" << std::hex << key.getHash() << " : 0x" << *hashPtr << " : 0x" << key.getHash64()
                        << std::dec << std::endl;
                    ++numCollisions;
                    assert(false);
                } else {
                    hashes.insert(key, key.getHash());
                }
                // track bit distribution counts to evaluate hash function randomness
                for (int k = 0; k < NUM_HASH_BITS; ++k) {
                    if (masks[k] & key.getHash()) {
                        ++bits[k];
                    }
                }
            }
        }
    }
    uint64_t msec = timer.getTimeMilliseconds();
    std::cout << msec << " msec with " << numCollisions << " collisions out of " << testCount << " hashes" << std::endl;

    // print out distribution of bits
    // ideally the numbers in each bin will be about the same
    for (int i = 0; i < NUM_HASH_BITS; ++i) {
        std::cout << "bit 0x" << std::hex << masks[i] << std::dec << " = " << bits[i] << std::endl;
    }
    QCOMPARE(numCollisions, 0);
#endif // MANUAL_TEST
}

void ShapeInfoTests::testBoxShape() {
    ShapeInfo info;
    glm::vec3 halfExtents(1.23f, 4.56f, 7.89f);
    info.setBox(halfExtents);
    HashKey key = info.getHash();

    const btCollisionShape* shape = ShapeFactory::createShapeFromInfo(info);
    QCOMPARE(shape != nullptr, true);

    ShapeInfo otherInfo = info;
    HashKey otherKey = otherInfo.getHash();
    QCOMPARE(key.getHash(), otherKey.getHash());

    delete shape;
}

void ShapeInfoTests::testSphereShape() {
    ShapeInfo info;
    float radius = 1.23f;
    info.setSphere(radius);
    HashKey key = info.getHash();

    const btCollisionShape* shape = ShapeFactory::createShapeFromInfo(info);
    QCOMPARE(shape != nullptr, true);

    ShapeInfo otherInfo = info;
    HashKey otherKey = otherInfo.getHash();
    QCOMPARE(key.getHash(), otherKey.getHash());

    delete shape;
}

void ShapeInfoTests::testCylinderShape() {
    /* TODO: reimplement Cylinder shape
    ShapeInfo info;
    float radius = 1.23f;
    float height = 4.56f;
    info.setCylinder(radius, height);
    HashKey key = info.getHash();

    btCollisionShape* shape = ShapeFactory::createShapeFromInfo(info);
    QCOMPARE(shape != nullptr, true);

    ShapeInfo otherInfo = info;
    HashKey otherKey = otherInfo.getHash();
    QCOMPARE(key.getHash(), otherKey.getHash());

    delete shape;
    */
}

void ShapeInfoTests::testCapsuleShape() {
    /* TODO: reimplement Capsule shape
    ShapeInfo info;
    float radius = 1.23f;
    float height = 4.56f;
    info.setCapsule(radius, height);
    HashKey key = info.getHash();

    btCollisionShape* shape = ShapeFactory::createShapeFromInfo(info);
    QCOMPARE(shape != nullptr, true);

    ShapeInfo otherInfo = info;
    HashKey otherKey = otherInfo.getHash();
    QCOMPARE(key.getHash(), otherKey.getHash());

    delete shape;
    */
}

