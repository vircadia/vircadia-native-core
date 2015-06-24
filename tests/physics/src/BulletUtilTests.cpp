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

//#include "PhysicsTestUtil.h"
#include <BulletUtil.h>
#include <NumericalConstants.h>

#include "BulletUtilTests.h"

// Constants
const glm::vec3 origin(0.0f);
const glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
const glm::vec3 zAxis(0.0f, 0.0f, 1.0f);


QTEST_MAIN(BulletUtilTests)

void BulletUtilTests::fromBulletToGLM() {
    btVector3 bV(1.23f, 4.56f, 7.89f);
    glm::vec3 gV = bulletToGLM(bV);
    
    QCOMPARE(gV.x, bV.getX());
    QCOMPARE(gV.y, bV.getY());
    QCOMPARE(gV.z, bV.getZ());

    float angle = 0.317f * PI;
    btVector3 axis(1.23f, 2.34f, 3.45f);
    axis.normalize();
    btQuaternion bQ(axis, angle);

    glm::quat gQ = bulletToGLM(bQ);
    QCOMPARE(gQ.x, bQ.getX());
    QCOMPARE(gQ.y, bQ.getY());
    QCOMPARE(gQ.z, bQ.getZ());
    QCOMPARE(gQ.w, bQ.getW());
}

void BulletUtilTests::fromGLMToBullet() {
    glm::vec3 gV(1.23f, 4.56f, 7.89f);
    btVector3 bV = glmToBullet(gV);
    
    QCOMPARE(gV.x, bV.getX());
    QCOMPARE(gV.y, bV.getY());
    QCOMPARE(gV.z, bV.getZ());

    float angle = 0.317f * PI;
    btVector3 axis(1.23f, 2.34f, 3.45f);
    axis.normalize();
    btQuaternion bQ(axis, angle);

    glm::quat gQ = bulletToGLM(bQ);
    QCOMPARE(gQ.x, bQ.getX());
    QCOMPARE(gQ.y, bQ.getY());
    QCOMPARE(gQ.z, bQ.getZ());
    QCOMPARE(gQ.w, bQ.getW());
}

//void BulletUtilTests::fooTest () {
//    
//    glm::vec3 a { 1, 0, 3 };
//    glm::vec3 b { 2, 0, 5 };
//    
////    QCOMPARE(10, 22);
//    QCOMPARE_WITH_ABS_ERROR(a, b, 1.0f);
//}
