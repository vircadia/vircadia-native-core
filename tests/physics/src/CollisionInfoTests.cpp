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


/*

static glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
static glm::vec3 xZxis(0.0f, 1.0f, 0.0f);
static glm::vec3 xYxis(0.0f, 0.0f, 1.0f);

void CollisionInfoTests::rotateThenTranslate() {
    CollisionInfo collision;
    collision._penetration = xAxis;
    collision._contactPoint = yAxis;
    collision._addedVelocity = xAxis + yAxis + zAxis;

    glm::quat rotation = glm::angleAxis(PI_OVER_TWO, zAxis);
    float distance = 3.0f;
    glm::vec3 translation = distance * yAxis;

    collision.rotateThenTranslate(rotation, translation);

    float error = glm::distance(collision._penetration, yAxis);
    if (error > EPSILON) {
        std::cout << __FILE__ << ":" << __LINE__ 
            << " ERROR: collision._penetration = " << collision._penetration
            << " but we expected " << yAxis
            << std::endl;
    } 

    glm::vec3 expectedContactPoint = -xAxis + translation;
    error = glm::distance(collision._contactPoint, expectedContactPoint);
    if (error > EPSILON) {
        std::cout << __FILE__ << ":" << __LINE__ 
            << " ERROR: collision._contactPoint = " << collision._contactPoint
            << " but we expected " << expectedContactPoint
            << std::endl;
    } 

    glm::vec3 expectedAddedVelocity = yAxis - xAxis + zAxis;
    error = glm::distance(collision._addedVelocity, expectedAddedVelocity);
    if (error > EPSILON) {
        std::cout << __FILE__ << ":" << __LINE__ 
            << " ERROR: collision._addedVelocity = " << collision._contactPoint
            << " but we expected " << expectedAddedVelocity
            << std::endl;
    } 
}

void CollisionInfoTests::translateThenRotate() {
    CollisionInfo collision;
    collision._penetration = xAxis;
    collision._contactPoint = yAxis;
    collision._addedVelocity = xAxis + yAxis + zAxis;

    glm::quat rotation = glm::angleAxis( -PI_OVER_TWO, zAxis);
    float distance = 3.0f;
    glm::vec3 translation = distance * yAxis;

    collision.translateThenRotate(translation, rotation);

    float error = glm::distance(collision._penetration, -yAxis);
    if (error > EPSILON) {
        std::cout << __FILE__ << ":" << __LINE__ 
            << " ERROR: collision._penetration = " << collision._penetration
            << " but we expected " << -yAxis
            << std::endl;
    } 

    glm::vec3 expectedContactPoint = (1.0f + distance) * xAxis;
    error = glm::distance(collision._contactPoint, expectedContactPoint);
    if (error > EPSILON) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: collision._contactPoint = " << collision._contactPoint
            << " but we expected " << expectedContactPoint
            << std::endl;
    } 

    glm::vec3 expectedAddedVelocity = - yAxis + xAxis + zAxis;
    error = glm::distance(collision._addedVelocity, expectedAddedVelocity);
    if (error > EPSILON) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: collision._addedVelocity = " << collision._contactPoint
            << " but we expected " << expectedAddedVelocity
            << std::endl;
    } 
}
*/

void CollisionInfoTests::runAllTests() {
//    CollisionInfoTests::rotateThenTranslate();
//    CollisionInfoTests::translateThenRotate();
}
