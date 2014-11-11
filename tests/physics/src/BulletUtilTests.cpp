//
//  BulletUtilTests.cpp
//  tests/physics/src
//
//  Created by Andrew Meadows on 2014.11.02
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>

#include <BulletUtil.h>
#include <SharedUtil.h>

#include "BulletUtilTests.h"

#ifdef USE_BULLET_PHYSICS
void BulletUtilTests::fromBulletToGLM() {
    btVector3 bV(1.23f, 4.56f, 7.89f);
    glm::vec3 gV;
    bulletToGLM(bV, gV);
    if (gV.x != bV.getX()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch bullet.x = " << bV.getX() << "  !=  glm.x = " << gV.x << std::endl;
    }
    if (gV.y != bV.getY()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch bullet.y = " << bV.getY() << "  !=  glm.y = " << gV.y << std::endl;
    }
    if (gV.z != bV.getZ()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch bullet.z = " << bV.getZ() << "  !=  glm.z = " << gV.z << std::endl;
    }

    float angle = 0.317f * PI;
    btVector3 axis(1.23f, 2.34f, 3.45f);
    axis.normalize();
    btQuaternion bQ(axis, angle);

    glm::quat gQ;
    bulletToGLM(bQ, gQ);
    if (gQ.x != bQ.getX()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch bullet.x = " << bQ.getX() << "  !=  glm.x = " << gQ.x << std::endl;
    }
    if (gQ.y != bQ.getY()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch bullet.y = " << bQ.getY() << "  !=  glm.y = " << gQ.y << std::endl;
    }
    if (gQ.z != bQ.getZ()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch bullet.z = " << bQ.getZ() << "  !=  glm.z = " << gQ.z << std::endl;
    }
    if (gQ.w != bQ.getW()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch bullet.w = " << bQ.getW() << "  !=  glm.w = " << gQ.w << std::endl;
    }
}

void BulletUtilTests::fromGLMToBullet() {
    glm::vec3 gV(1.23f, 4.56f, 7.89f);
    btVector3 bV;
    glmToBullet(gV, bV);
    if (gV.x != bV.getX()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch glm.x = " << gV.x << "  !=  bullet.x = " << bV.getX() << std::endl;
    }
    if (gV.y != bV.getY()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch glm.y = " << gV.y << "  !=  bullet.y = " << bV.getY() << std::endl;
    }
    if (gV.z != bV.getZ()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch glm.z = " << gV.z << "  !=  bullet.z = " << bV.getZ() << std::endl;
    }

    float angle = 0.317f * PI;
    btVector3 axis(1.23f, 2.34f, 3.45f);
    axis.normalize();
    btQuaternion bQ(axis, angle);

    glm::quat gQ;
    bulletToGLM(bQ, gQ);
    if (gQ.x != bQ.getX()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch glm.x = " << gQ.x << "  !=  bullet.x = " << bQ.getX() << std::endl;
    }
    if (gQ.y != bQ.getY()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch glm.y = " << gQ.y << "  !=  bullet.y = " << bQ.getY() << std::endl;
    }
    if (gQ.z != bQ.getZ()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch glm.z = " << gQ.z << "  !=  bullet.z = " << bQ.getZ() << std::endl;
    }
    if (gQ.w != bQ.getW()) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: x mismatch glm.w = " << gQ.w << "  !=  bullet.w = " << bQ.getW() << std::endl;
    }
}

void BulletUtilTests::runAllTests() {
    fromBulletToGLM();
    fromGLMToBullet();
}

#else // USE_BULLET_PHYSICS
void BulletUtilTests::fromBulletToGLM() {
}

void BulletUtilTests::fromGLMToBullet() {
}

void BulletUtilTests::runAllTests() {
}
#endif // USE_BULLET_PHYSICS
