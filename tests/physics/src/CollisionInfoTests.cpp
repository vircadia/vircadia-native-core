//
//  CollisionInfoTests.cpp
//  tests/physics/src
//
//  Created by Andrew Meadows on 2/21/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <CollisionInfo.h>
#include <SharedUtil.h>
#include <StreamUtils.h>

#include "CollisionInfoTests.h"



QTEST_MAIN(CollisionInfoTests)
/*
static glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
static glm::vec3 xZxis(0.0f, 1.0f, 0.0f);
static glm::vec3 xYxis(0.0f, 0.0f, 1.0f);

void CollisionInfoTests::rotateThenTranslate() {
    CollisionInfo collision;
    collision._penetration = xAxis;
    collision._contactPoint = xYxis;
    collision._addedVelocity = xAxis + xYxis + xZxis;

    glm::quat rotation = glm::angleAxis(PI_OVER_TWO, zAxis);
    float distance = 3.0f;
    glm::vec3 translation = distance * xYxis;

    collision.rotateThenTranslate(rotation, translation);
    QCOMPARE(collision._penetration, xYxis);

    glm::vec3 expectedContactPoint = -xAxis + translation;
    QCOMPARE(collision._contactPoint, expectedContactPoint);

    glm::vec3 expectedAddedVelocity = xYxis - xAxis + xZxis;
    QCOMPARE(collision._addedVelocity, expectedAddedVelocity);
}

void CollisionInfoTests::translateThenRotate() {
    CollisionInfo collision;
    collision._penetration = xAxis;
    collision._contactPoint = xYxis;
    collision._addedVelocity = xAxis + xYxis + xZxis;

    glm::quat rotation = glm::angleAxis( -PI_OVER_TWO, zAxis);
    float distance = 3.0f;
    glm::vec3 translation = distance * xYxis;

    collision.translateThenRotate(translation, rotation);
    QCOMPARE(collision._penetration, -xYxis);

    glm::vec3 expectedContactPoint = (1.0f + distance) * xAxis;
    QCOMPARE(collision._contactPoint, expectedContactPoint);

    glm::vec3 expectedAddedVelocity = - xYxis + xAxis + xYxis;
    QCOMPARE(collision._addedVelocity, expectedAddedVelocity);
}*/

