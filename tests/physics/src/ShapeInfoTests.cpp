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
#include <ShapeInfo.h>
#include <StreamUtils.h>

#include "ShapeInfoTests.h"

void ShapeInfoTests::testHashFunctions() {
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

void ShapeInfoTests::testBoxShape() {
#ifdef USE_BULLET_PHYSICS
    ShapeInfo info;
    btVector3 halfExtents(1.23f, 4.56f, 7.89f);
    info.setBox(halfExtents);
    int hash = info.computeHash();
    int hash2 = info.computeHash2();

    btCollisionShape* shape = info.createShape();
    if (!shape) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: NULL Box shape" << std::endl;
    }

    ShapeInfo otherInfo;
    otherInfo.collectInfo(shape);

    int otherHash = otherInfo.computeHash();
    if (hash != otherHash) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Box shape hash = " << hash << " but found hash = " << otherHash << std::endl;
    }

    int otherHash2= otherInfo.computeHash2();
    if (hash2 != otherHash2) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Box shape hash2 = " << hash2 << " but found hash2 = " << otherHash2 << std::endl;
    }

    delete shape;
#endif // USE_BULLET_PHYSICS
}

void ShapeInfoTests::testSphereShape() {
#ifdef USE_BULLET_PHYSICS
    ShapeInfo info;
    float radius = 1.23f;
    info.setSphere(radius);
    int hash = info.computeHash();
    int hash2 = info.computeHash2();

    btCollisionShape* shape = info.createShape();

    ShapeInfo otherInfo;
    otherInfo.collectInfo(shape);

    int otherHash = otherInfo.computeHash();
    if (hash != otherHash) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Sphere shape hash = " << hash << " but found hash = " << otherHash << std::endl;
    }
    
    int otherHash2 = otherInfo.computeHash2();
    if (hash2 != otherHash2) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Sphere shape hash2 = " << hash2 << " but found hash2 = " << otherHash2 << std::endl;
    }

    delete shape;
#endif // USE_BULLET_PHYSICS
}

void ShapeInfoTests::testCylinderShape() {
#ifdef USE_BULLET_PHYSICS
    ShapeInfo info;
    float radius = 1.23f;
    float height = 4.56f;
    info.setCylinder(radius, height);
    int hash = info.computeHash();
    int hash2 = info.computeHash2();

    btCollisionShape* shape = info.createShape();

    ShapeInfo otherInfo;
    otherInfo.collectInfo(shape);

    int otherHash = otherInfo.computeHash();
    if (hash != otherHash) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Cylinder shape hash = " << hash << " but found hash = " << otherHash << std::endl;
    }

    int otherHash2 = otherInfo.computeHash2();
    if (hash2 != otherHash2) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Cylinder shape hash2 = " << hash2 << " but found hash2 = " << otherHash2 << std::endl;
    }

    delete shape;
#endif // USE_BULLET_PHYSICS
}

void ShapeInfoTests::testCapsuleShape() {
#ifdef USE_BULLET_PHYSICS
    ShapeInfo info;
    float radius = 1.23f;
    float height = 4.56f;
    info.setCapsule(radius, height);
    int hash = info.computeHash();
    int hash2 = info.computeHash2();

    btCollisionShape* shape = info.createShape();

    ShapeInfo otherInfo;
    otherInfo.collectInfo(shape);

    int otherHash = otherInfo.computeHash();
    if (hash != otherHash) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Capsule shape hash = " << hash << " but found hash = " << otherHash << std::endl;
    }

    int otherHash2 = otherInfo.computeHash2();
    if (hash2 != otherHash2) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected Capsule shape hash2 = " << hash2 << " but found hash2 = " << otherHash2 << std::endl;
    }

    delete shape;
#endif // USE_BULLET_PHYSICS
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
