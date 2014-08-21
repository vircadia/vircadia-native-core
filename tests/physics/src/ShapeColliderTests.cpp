//
//  ShapeColliderTests.cpp
//  tests/physics/src
//
//  Created by Andrew Meadows on 02/21/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

//#include <stdio.h>
#include <iostream>
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <CapsuleShape.h>
#include <CollisionInfo.h>
#include <PlaneShape.h>
#include <ShapeCollider.h>
#include <SharedUtil.h>
#include <SphereShape.h>
#include <StreamUtils.h>

#include "ShapeColliderTests.h"

const glm::vec3 origin(0.0f);
static const glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
static const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
static const glm::vec3 zAxis(0.0f, 0.0f, 1.0f);

void ShapeColliderTests::sphereMissesSphere() {
    // non-overlapping spheres of unequal size
    float radiusA = 7.0f;
    float radiusB = 3.0f;
    float alpha = 1.2f;
    float beta = 1.3f;
    glm::vec3 offsetDirection = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
    float offsetDistance = alpha * radiusA + beta * radiusB;

    SphereShape sphereA(radiusA, origin);
    SphereShape sphereB(radiusB, offsetDistance * offsetDirection);
    CollisionList collisions(16);

    // collide A to B...
    {
        bool touching = ShapeCollider::collideShapes(&sphereA, &sphereB, collisions);
        if (touching) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: sphereA and sphereB should NOT touch" << std::endl;
        }
    }

    // collide B to A...
    {
        bool touching = ShapeCollider::collideShapes(&sphereB, &sphereA, collisions);
        if (touching) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: sphereA and sphereB should NOT touch" << std::endl;
        }
    }

    // also test shapeShape
    {
        bool touching = ShapeCollider::collideShapes(&sphereB, &sphereA, collisions);
        if (touching) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: sphereA and sphereB should NOT touch" << std::endl;
        }
    }

    if (collisions.size() > 0) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected empty collision list but size is " << collisions.size()
            << std::endl;
    }
}

void ShapeColliderTests::sphereTouchesSphere() {
    // overlapping spheres of unequal size
    float radiusA = 7.0f;
    float radiusB = 3.0f;
    float alpha = 0.2f;
    float beta = 0.3f;
    glm::vec3 offsetDirection = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
    float offsetDistance = alpha * radiusA + beta * radiusB;
    float expectedPenetrationDistance = (1.0f - alpha) * radiusA + (1.0f - beta) * radiusB;
    glm::vec3 expectedPenetration = expectedPenetrationDistance * offsetDirection;

    SphereShape sphereA(radiusA, origin);
    SphereShape sphereB(radiusB, offsetDistance * offsetDirection);
    CollisionList collisions(16);
    int numCollisions = 0;

    // collide A to B...
    {
        bool touching = ShapeCollider::collideShapes(&sphereA, &sphereB, collisions);
        if (!touching) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: sphereA and sphereB should touch" << std::endl;
        } else {
            ++numCollisions;
        }

        // verify state of collisions
        if (numCollisions != collisions.size()) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: expected collisions size of " << numCollisions << " but actual size is " << collisions.size()
                << std::endl;
        }
        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        if (!collision) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: null collision" << std::endl;
            return;
        }
    
        // penetration points from sphereA into sphereB
        float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration;
        }
    
        // contactPoint is on surface of sphereA
        glm::vec3 AtoB = sphereB.getTranslation() - sphereA.getTranslation();
        glm::vec3 expectedContactPoint = sphereA.getTranslation() + radiusA * glm::normalize(AtoB);
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint;
        }
    }

    // collide B to A...
    {
        bool touching = ShapeCollider::collideShapes(&sphereB, &sphereA, collisions);
        if (!touching) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: sphereA and sphereB should touch" << std::endl;
        } else {
            ++numCollisions;
        }
    
        // penetration points from sphereA into sphereB
        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        float inaccuracy = glm::length(collision->_penetration + expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration;
        }
    
        // contactPoint is on surface of sphereA
        glm::vec3 BtoA = sphereA.getTranslation() - sphereB.getTranslation();
        glm::vec3 expectedContactPoint = sphereB.getTranslation() + radiusB * glm::normalize(BtoA);
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint;
        }
    }
}

void ShapeColliderTests::sphereMissesCapsule() {
    // non-overlapping sphere and capsule
    float radiusA = 1.5f;
    float radiusB = 2.3f;
    float totalRadius = radiusA + radiusB;
    float halfHeightB = 1.7f;
    float axialOffset = totalRadius + 1.1f * halfHeightB;
    float radialOffset = 1.2f * radiusA + 1.3f * radiusB;
    
    SphereShape sphereA(radiusA);
    CapsuleShape capsuleB(radiusB, halfHeightB);

    // give the capsule some arbirary transform
    float angle = 37.8f;
    glm::vec3 axis = glm::normalize( glm::vec3(-7.0f, 2.8f, 9.3f) );
    glm::quat rotation = glm::angleAxis(angle, axis);
    glm::vec3 translation(15.1f, -27.1f, -38.6f);
    capsuleB.setRotation(rotation);
    capsuleB.setTranslation(translation);

    CollisionList collisions(16);

    // walk sphereA along the local yAxis next to, but not touching, capsuleB
    glm::vec3 localStartPosition(radialOffset, axialOffset, 0.0f);
    int numberOfSteps = 10;
    float delta = 1.3f * (totalRadius + halfHeightB) / (numberOfSteps - 1);
    for (int i = 0; i < numberOfSteps; ++i) {
        // translate sphereA into world-frame
        glm::vec3 localPosition = localStartPosition + ((float)i * delta) * yAxis;
        sphereA.setTranslation(rotation * localPosition + translation);

        // sphereA agains capsuleB
        if (ShapeCollider::collideShapes(&sphereA, &capsuleB, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: sphere and capsule should NOT touch"
                << std::endl;
        }

        // capsuleB against sphereA
        if (ShapeCollider::collideShapes(&capsuleB, &sphereA, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: sphere and capsule should NOT touch"
                << std::endl;
        }
    }

    if (collisions.size() > 0) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected empty collision list but size is " << collisions.size()
            << std::endl;
    }
}

void ShapeColliderTests::sphereTouchesCapsule() {
    // overlapping sphere and capsule
    float radiusA = 2.0f;
    float radiusB = 1.0f;
    float totalRadius = radiusA + radiusB;
    float halfHeightB = 2.0f;
    float alpha = 0.5f;
    float beta = 0.5f;
    float radialOffset = alpha * radiusA + beta * radiusB;
    
    SphereShape sphereA(radiusA);
    CapsuleShape capsuleB(radiusB, halfHeightB);

    CollisionList collisions(16);
    int numCollisions = 0;

    {   // sphereA collides with capsuleB's cylindrical wall
        sphereA.setTranslation(radialOffset * xAxis);

        if (!ShapeCollider::collideShapes(&sphereA, &capsuleB, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: sphere and capsule should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
    
        // penetration points from sphereA into capsuleB
        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        glm::vec3 expectedPenetration = (radialOffset - totalRadius) * xAxis;
        float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration;
        }
    
        // contactPoint is on surface of sphereA
        glm::vec3 expectedContactPoint = sphereA.getTranslation() - radiusA * xAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint;
        }

        // capsuleB collides with sphereA
        if (!ShapeCollider::collideShapes(&capsuleB, &sphereA, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and sphere should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
    
        // penetration points from sphereA into capsuleB
        collision = collisions.getCollision(numCollisions - 1);
        expectedPenetration = - (radialOffset - totalRadius) * xAxis;
        inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration;
        }
    
        // contactPoint is on surface of capsuleB
        glm::vec3 BtoA = sphereA.getTranslation() - capsuleB.getTranslation();
        glm::vec3 closestApproach = capsuleB.getTranslation() + glm::dot(BtoA, yAxis) * yAxis;
        expectedContactPoint = closestApproach + radiusB * glm::normalize(BtoA - closestApproach);
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint;
        }
    }
    {   // sphereA hits end cap at axis
        glm::vec3 axialOffset = (halfHeightB + alpha * radiusA + beta * radiusB) * yAxis;
        sphereA.setTranslation(axialOffset * yAxis);
        
        if (!ShapeCollider::collideShapes(&sphereA, &capsuleB, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: sphere and capsule should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
    
        // penetration points from sphereA into capsuleB
        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        glm::vec3 expectedPenetration = - ((1.0f - alpha) * radiusA + (1.0f - beta) * radiusB) * yAxis;
        float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration;
        }
    
        // contactPoint is on surface of sphereA
        glm::vec3 expectedContactPoint = sphereA.getTranslation() - radiusA * yAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint;
        }

        // capsuleB collides with sphereA
        if (!ShapeCollider::collideShapes(&capsuleB, &sphereA, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and sphere should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
    
        // penetration points from sphereA into capsuleB
        collision = collisions.getCollision(numCollisions - 1);
        expectedPenetration = ((1.0f - alpha) * radiusA + (1.0f - beta) * radiusB) * yAxis;
        inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration;
        }
    
        // contactPoint is on surface of capsuleB
        glm::vec3 endPoint;
        capsuleB.getEndPoint(endPoint);
        expectedContactPoint = endPoint + radiusB * yAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint;
        }
    }
    {   // sphereA hits start cap at axis
        glm::vec3 axialOffset = - (halfHeightB + alpha * radiusA + beta * radiusB) * yAxis;
        sphereA.setTranslation(axialOffset * yAxis);
        
        if (!ShapeCollider::collideShapes(&sphereA, &capsuleB, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: sphere and capsule should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
    
        // penetration points from sphereA into capsuleB
        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        glm::vec3 expectedPenetration = ((1.0f - alpha) * radiusA + (1.0f - beta) * radiusB) * yAxis;
        float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration;
        }
    
        // contactPoint is on surface of sphereA
        glm::vec3 expectedContactPoint = sphereA.getTranslation() + radiusA * yAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint;
        }

        // capsuleB collides with sphereA
        if (!ShapeCollider::collideShapes(&capsuleB, &sphereA, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and sphere should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
    
        // penetration points from sphereA into capsuleB
        collision = collisions.getCollision(numCollisions - 1);
        expectedPenetration = - ((1.0f - alpha) * radiusA + (1.0f - beta) * radiusB) * yAxis;
        inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration;
        }
    
        // contactPoint is on surface of capsuleB
        glm::vec3 startPoint;
        capsuleB.getStartPoint(startPoint);
        expectedContactPoint = startPoint - radiusB * yAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint;
        }
    }
    if (collisions.size() != numCollisions) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected " << numCollisions << " collisions but actual number is " << collisions.size()
            << std::endl;
    }
}

void ShapeColliderTests::capsuleMissesCapsule() {
    // non-overlapping capsules
    float radiusA = 2.0f;
    float halfHeightA = 3.0f;
    float radiusB = 3.0f;
    float halfHeightB = 4.0f;

    float totalRadius = radiusA + radiusB;
    float totalHalfLength = totalRadius + halfHeightA + halfHeightB;

    CapsuleShape capsuleA(radiusA, halfHeightA);
    CapsuleShape capsuleB(radiusB, halfHeightB);

    CollisionList collisions(16);

    // side by side
    capsuleB.setTranslation((1.01f * totalRadius) * xAxis);
    if (ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
    {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: capsule and capsule should NOT touch"
            << std::endl;
    }
    if (ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
    {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: capsule and capsule should NOT touch"
            << std::endl;
    }

    // end to end
    capsuleB.setTranslation((1.01f * totalHalfLength) * xAxis);
    if (ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
    {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: capsule and capsule should NOT touch"
            << std::endl;
    }
    if (ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
    {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: capsule and capsule should NOT touch"
            << std::endl;
    }

    // rotate B and move it to the side
    glm::quat rotation = glm::angleAxis(PI_OVER_TWO, zAxis);
    capsuleB.setRotation(rotation);
    capsuleB.setTranslation((1.01f * (totalRadius + capsuleB.getHalfHeight())) * xAxis);
    if (ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
    {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: capsule and capsule should NOT touch"
            << std::endl;
    }
    if (ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
    {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: capsule and capsule should NOT touch"
            << std::endl;
    }

    if (collisions.size() > 0) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected empty collision list but size is " << collisions.size()
            << std::endl;
    }
}

void ShapeColliderTests::capsuleTouchesCapsule() {
    // overlapping capsules
    float radiusA = 2.0f;
    float halfHeightA = 3.0f;
    float radiusB = 3.0f;
    float halfHeightB = 4.0f;

    float totalRadius = radiusA + radiusB;
    float totalHalfLength = totalRadius + halfHeightA + halfHeightB;

    CapsuleShape capsuleA(radiusA, halfHeightA);
    CapsuleShape capsuleB(radiusB, halfHeightB);

    CollisionList collisions(16);
    int numCollisions = 0;

    { // side by side
        capsuleB.setTranslation((0.99f * totalRadius) * xAxis);
        if (!ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and capsule should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
        if (!ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and capsule should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
    }

    { // end to end
        capsuleB.setTranslation((0.99f * totalHalfLength) * yAxis);

        if (!ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and capsule should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
        if (!ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and capsule should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
    }

    { // rotate B and move it to the side
        glm::quat rotation = glm::angleAxis(PI_OVER_TWO, zAxis);
        capsuleB.setRotation(rotation);
        capsuleB.setTranslation((0.99f * (totalRadius + capsuleB.getHalfHeight())) * xAxis);

        if (!ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and capsule should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
        if (!ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and capsule should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
    }

    { // again, but this time check collision details
        float overlap = 0.1f;
        glm::quat rotation = glm::angleAxis(PI_OVER_TWO, zAxis);
        capsuleB.setRotation(rotation);
        glm::vec3 positionB = ((totalRadius + capsuleB.getHalfHeight()) - overlap) * xAxis;
        capsuleB.setTranslation(positionB);

        // capsuleA vs capsuleB
        if (!ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and capsule should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
    
        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        glm::vec3 expectedPenetration = overlap * xAxis;
        float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration;
        }
    
        glm::vec3 expectedContactPoint = capsuleA.getTranslation() + radiusA * xAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint;
        }

        // capsuleB vs capsuleA
        if (!ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and capsule should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
    
        collision = collisions.getCollision(numCollisions - 1);
        expectedPenetration = - overlap * xAxis;
        inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration 
                << std::endl;
        }
    
        expectedContactPoint = capsuleB.getTranslation() - (radiusB + halfHeightB) * xAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint 
                << std::endl;
        }
    }

    { // collide cylinder wall against cylinder wall
        float overlap = 0.137f;
        float shift = 0.317f * halfHeightA;
        glm::quat rotation = glm::angleAxis(PI_OVER_TWO, zAxis);
        capsuleB.setRotation(rotation);
        glm::vec3 positionB = (totalRadius - overlap) * zAxis + shift * yAxis;
        capsuleB.setTranslation(positionB);

        // capsuleA vs capsuleB
        if (!ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and capsule should touch"
                << std::endl;
        } else {
            ++numCollisions;
        }
    
        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        glm::vec3 expectedPenetration = overlap * zAxis;
        float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration 
                << std::endl;
        }
    
        glm::vec3 expectedContactPoint = capsuleA.getTranslation() + radiusA * zAxis + shift * yAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint 
                << std::endl;
        }
    }
}

void ShapeColliderTests::sphereTouchesAACubeFaces() {
    CollisionList collisions(16);
    
    glm::vec3 cubeCenter(1.23f, 4.56f, 7.89f);
    float cubeSide = 2.34f;

    float sphereRadius = 1.13f;
    glm::vec3 sphereCenter(0.0f);
    SphereShape sphere(sphereRadius, sphereCenter);

    QVector<glm::vec3> axes;
    axes.push_back(xAxis);
    axes.push_back(-xAxis);
    axes.push_back(yAxis);
    axes.push_back(-yAxis);
    axes.push_back(zAxis);
    axes.push_back(-zAxis);

    for (int i = 0; i < axes.size(); ++i) {
        glm::vec3 axis = axes[i];
        // outside
        {
            collisions.clear();
            float overlap = 0.25f;
            float sphereOffset = 0.5f * cubeSide + sphereRadius - overlap;
            sphereCenter = cubeCenter + sphereOffset * axis;
            sphere.setTranslation(sphereCenter);
    
            if (!ShapeCollider::sphereAACube(&sphere, cubeCenter, cubeSide, collisions)){
                std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should collide with cube.  axis = " << axis << std::endl;
            }
            CollisionInfo* collision = collisions[0];
            if (!collision) {
                std::cout << __FILE__ << ":" << __LINE__ << " ERROR: no CollisionInfo. axis = " << axis << std::endl;
            }
    
            glm::vec3 expectedPenetration  = - overlap * axis;
            if (glm::distance(expectedPenetration, collision->_penetration) > EPSILON) {
                std::cout << __FILE__ << ":" << __LINE__ << " ERROR: penetration = " << collision->_penetration 
                    << "  expected " << expectedPenetration 
                    << "  axis = " << axis
                    << std::endl;
            }
    
            glm::vec3 expectedContact = sphereCenter - sphereRadius * axis;
            if (glm::distance(expectedContact, collision->_contactPoint) > EPSILON) {
                std::cout << __FILE__ << ":" << __LINE__ << " ERROR: contactaPoint = " << collision->_contactPoint 
                    << "  expected " << expectedContact 
                    << "  axis = " << axis
                    << std::endl;
            }
        }

        // inside
        {
            collisions.clear();
            float overlap = 1.25f * sphereRadius;
            float sphereOffset = 0.5f * cubeSide + sphereRadius - overlap;
            sphereCenter = cubeCenter + sphereOffset * axis;
            sphere.setTranslation(sphereCenter);
    
            if (!ShapeCollider::sphereAACube(&sphere, cubeCenter, cubeSide, collisions)){
                std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should collide with cube." 
                    << "  axis = " << axis
                    << std::endl;
            }
            CollisionInfo* collision = collisions[0];
            if (!collision) {
                std::cout << __FILE__ << ":" << __LINE__ << " ERROR: no CollisionInfo on y-axis."
                    << "  axis = " << axis 
                    << std::endl;
            }
    
            glm::vec3 expectedPenetration  = - overlap * axis;
            if (glm::distance(expectedPenetration, collision->_penetration) > EPSILON) {
                std::cout << __FILE__ << ":" << __LINE__ << " ERROR: penetration = " << collision->_penetration 
                    << "  expected " << expectedPenetration
                    << "  axis = " << axis 
                    << std::endl;
            }
    
            glm::vec3 expectedContact = sphereCenter - sphereRadius * axis;
            if (glm::distance(expectedContact, collision->_contactPoint) > EPSILON) {
                std::cout << __FILE__ << ":" << __LINE__ << " ERROR: contactaPoint = " << collision->_contactPoint 
                    << "  expected " << expectedContact
                    << "  axis = " << axis 
                    << std::endl;
            }
        }
    }
}

void ShapeColliderTests::sphereTouchesAACubeEdges() {
    CollisionList collisions(20);
    
    glm::vec3 cubeCenter(0.0f, 0.0f, 0.0f);
    float cubeSide = 2.0f;

    float sphereRadius = 1.0f;
    glm::vec3 sphereCenter(0.0f);
    SphereShape sphere(sphereRadius, sphereCenter);

    QVector<glm::vec3> axes;
    // edges
    axes.push_back(glm::vec3(0.0f, 1.0f, 1.0f));
    axes.push_back(glm::vec3(0.0f, 1.0f, -1.0f));
    axes.push_back(glm::vec3(0.0f, -1.0f, 1.0f));
    axes.push_back(glm::vec3(0.0f, -1.0f, -1.0f));
    axes.push_back(glm::vec3(1.0f, 1.0f, 0.0f));
    axes.push_back(glm::vec3(1.0f, -1.0f, 0.0f));
    axes.push_back(glm::vec3(-1.0f, 1.0f, 0.0f));
    axes.push_back(glm::vec3(-1.0f, -1.0f, 0.0f));
    axes.push_back(glm::vec3(1.0f, 0.0f, 1.0f));
    axes.push_back(glm::vec3(1.0f, 0.0f, -1.0f));
    axes.push_back(glm::vec3(-1.0f, 0.0f, 1.0f));
    axes.push_back(glm::vec3(-1.0f, 0.0f, -1.0f));
    // and corners
    axes.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
    axes.push_back(glm::vec3(1.0f, 1.0f, -1.0f));
    axes.push_back(glm::vec3(1.0f, -1.0f, 1.0f));
    axes.push_back(glm::vec3(1.0f, -1.0f, -1.0f));
    axes.push_back(glm::vec3(-1.0f, 1.0f, 1.0f));
    axes.push_back(glm::vec3(-1.0f, 1.0f, -1.0f));
    axes.push_back(glm::vec3(-1.0f, -1.0f, 1.0f));
    axes.push_back(glm::vec3(-1.0f, -1.0f, -1.0f));

    for (int i =0; i < axes.size(); ++i) {
        glm::vec3 axis = axes[i];
        float lengthAxis = glm::length(axis);
        axis /= lengthAxis;
        float overlap = 0.25f;
    
        sphereCenter = cubeCenter + (lengthAxis * 0.5f * cubeSide + sphereRadius - overlap) * axis;
        sphere.setTranslation(sphereCenter);
    
        if (!ShapeCollider::sphereAACube(&sphere, cubeCenter, cubeSide, collisions)){
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should collide with cube.  axis = " << axis << std::endl;
        }
        CollisionInfo* collision = collisions[i];
        if (!collision) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: no CollisionInfo. axis = " << axis << std::endl;
        }
    
        glm::vec3 expectedPenetration  = - overlap * axis;
        if (glm::distance(expectedPenetration, collision->_penetration) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: penetration = " << collision->_penetration 
                << "  expected " << expectedPenetration 
                << "  axis = " << axis
                << std::endl;
        }
    
        glm::vec3 expectedContact = sphereCenter - sphereRadius * axis;
        if (glm::distance(expectedContact, collision->_contactPoint) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: contactaPoint = " << collision->_contactPoint 
                << "  expected " << expectedContact 
                << "  axis = " << axis
                << std::endl;
        }
    }
}

void ShapeColliderTests::sphereMissesAACube() {
    CollisionList collisions(16);
    
    glm::vec3 cubeCenter(1.23f, 4.56f, 7.89f);
    float cubeSide = 2.0f;

    float sphereRadius = 1.0f;
    glm::vec3 sphereCenter(0.0f);
    SphereShape sphere(sphereRadius, sphereCenter);

    float sphereOffset = (0.5f * cubeSide + sphereRadius + 0.25f);

    // top
    sphereCenter = cubeCenter + sphereOffset * yAxis;
    sphere.setTranslation(sphereCenter);
    if (ShapeCollider::sphereAACube(&sphere, cubeCenter, cubeSide, collisions)){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should NOT collide with cube" << std::endl;
    }
    
    // bottom
    sphereCenter = cubeCenter - sphereOffset * yAxis;
    sphere.setTranslation(sphereCenter);
    if (ShapeCollider::sphereAACube(&sphere, cubeCenter, cubeSide, collisions)){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should NOT collide with cube" << std::endl;
    }

    // left
    sphereCenter = cubeCenter + sphereOffset * xAxis;
    sphere.setTranslation(sphereCenter);
    if (ShapeCollider::sphereAACube(&sphere, cubeCenter, cubeSide, collisions)){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should NOT collide with cube" << std::endl;
    }

    // right
    sphereCenter = cubeCenter - sphereOffset * xAxis;
    sphere.setTranslation(sphereCenter);
    if (ShapeCollider::sphereAACube(&sphere, cubeCenter, cubeSide, collisions)){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should NOT collide with cube" << std::endl;
    }

    // forward
    sphereCenter = cubeCenter + sphereOffset * zAxis;
    sphere.setTranslation(sphereCenter);
    if (ShapeCollider::sphereAACube(&sphere, cubeCenter, cubeSide, collisions)){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should NOT collide with cube" << std::endl;
    }

    // back
    sphereCenter = cubeCenter - sphereOffset * zAxis;
    sphere.setTranslation(sphereCenter);
    if (ShapeCollider::sphereAACube(&sphere, cubeCenter, cubeSide, collisions)){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should NOT collide with cube" << std::endl;
    }
}

void ShapeColliderTests::rayHitsSphere() {
    float startDistance = 3.0f;
    glm::vec3 rayStart(-startDistance, 0.0f, 0.0f);
    glm::vec3 rayDirection(1.0f, 0.0f, 0.0f);

    float radius = 1.0f;
    glm::vec3 center(0.0f);

    SphereShape sphere(radius, center);

    // very simple ray along xAxis
    {
        float distance = FLT_MAX;
        if (!sphere.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should intersect sphere" << std::endl;
        }
    
        float expectedDistance = startDistance - radius;
        float relativeError = fabsf(distance - expectedDistance) / startDistance;
        if (relativeError > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray sphere intersection distance error = " << relativeError << std::endl;
        }
    }

    // ray along a diagonal axis
    {
        rayStart = glm::vec3(startDistance, startDistance, 0.0f);
        rayDirection = - glm::normalize(rayStart);
    
        float distance = FLT_MAX;
        if (!sphere.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should intersect sphere" << std::endl;
        }
    
        float expectedDistance = SQUARE_ROOT_OF_2 * startDistance - radius;
        float relativeError = fabsf(distance - expectedDistance) / startDistance;
        if (relativeError > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray sphere intersection distance error = " << relativeError << std::endl;
        }
    }

    // rotated and displaced ray and sphere
    {
        startDistance = 7.41f;
        radius = 3.917f;

        glm::vec3 axis = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
        glm::quat rotation = glm::angleAxis(0.987654321f, axis);
        glm::vec3 translation(35.7f, 2.46f, -1.97f);

        glm::vec3 unrotatedRayDirection(-1.0f, 0.0f, 0.0f);
        glm::vec3 untransformedRayStart(startDistance, 0.0f, 0.0f);

        rayStart = rotation * (untransformedRayStart + translation);
        rayDirection = rotation * unrotatedRayDirection;

        sphere.setRadius(radius);
        sphere.setTranslation(rotation * translation);
    
        float distance = FLT_MAX;
        if (!sphere.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should intersect sphere" << std::endl;
        }
    
        float expectedDistance = startDistance - radius;
        float relativeError = fabsf(distance - expectedDistance) / startDistance;
        if (relativeError > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray sphere intersection distance error = " << relativeError << std::endl;
        }
    }
}

void ShapeColliderTests::rayBarelyHitsSphere() {
    float radius = 1.0f;
    glm::vec3 center(0.0f);
    float delta = 2.0f * EPSILON;

    float startDistance = 3.0f;
    glm::vec3 rayStart(-startDistance, radius - delta, 0.0f);
    glm::vec3 rayDirection(1.0f, 0.0f, 0.0f);

    SphereShape sphere(radius, center);

    // very simple ray along xAxis
    float distance = FLT_MAX;
    if (!sphere.findRayIntersection(rayStart, rayDirection, distance)) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should just barely hit sphere" << std::endl;
    }

    // translate and rotate the whole system... 
    glm::vec3 axis = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
    glm::quat rotation = glm::angleAxis(0.987654321f, axis);
    glm::vec3 translation(35.7f, 2.46f, -1.97f);

    rayStart = rotation * (rayStart + translation);
    rayDirection = rotation * rayDirection;
    sphere.setTranslation(rotation * translation);

    // ...and test again
    distance = FLT_MAX;
    if (!sphere.findRayIntersection(rayStart, rayDirection, distance)) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should just barely hit sphere" << std::endl;
    }
}


void ShapeColliderTests::rayBarelyMissesSphere() {
    // same as the barely-hits case, but this time we move the ray away from sphere
    float radius = 1.0f;
    glm::vec3 center(0.0f);
    float delta = 2.0f * EPSILON;

    float startDistance = 3.0f;
    glm::vec3 rayStart(-startDistance, radius + delta, 0.0f);
    glm::vec3 rayDirection(1.0f, 0.0f, 0.0f);

    SphereShape sphere(radius, center);

    // very simple ray along xAxis
    float distance = FLT_MAX;
    if (sphere.findRayIntersection(rayStart, rayDirection, distance)) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should just barely miss sphere" << std::endl;
    }
    if (distance != FLT_MAX) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss" << std::endl;
    }

    // translate and rotate the whole system... 
    glm::vec3 axis = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
    glm::quat rotation = glm::angleAxis(0.987654321f, axis);
    glm::vec3 translation(35.7f, 2.46f, -1.97f);

    rayStart = rotation * (rayStart + translation);
    rayDirection = rotation * rayDirection;
    sphere.setTranslation(rotation * translation);

    // ...and test again
    distance = FLT_MAX;
    if (sphere.findRayIntersection(rayStart, rayDirection, distance)) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should just barely miss sphere" << std::endl;
    }
    if (distance != FLT_MAX) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss" << std::endl;
    }
}

void ShapeColliderTests::rayHitsCapsule() {
    float startDistance = 3.0f;
    float radius = 1.0f;
    float halfHeight = 2.0f;
    glm::vec3 center(0.0f);
    CapsuleShape capsule(radius, halfHeight);

    { // simple test along xAxis 
        // toward capsule center
        glm::vec3 rayStart(startDistance, 0.0f, 0.0f);
        glm::vec3 rayDirection(-1.0f, 0.0f, 0.0f);
        float distance = FLT_MAX;
        if (!capsule.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit capsule" << std::endl;
        }
        float expectedDistance = startDistance - radius;
        float relativeError = fabsf(distance - expectedDistance) / startDistance;
        if (relativeError > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray capsule intersection distance error = " << relativeError << std::endl;
        }

        // toward top of cylindrical wall
        rayStart.y = halfHeight;
        distance = FLT_MAX;
        if (!capsule.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit capsule" << std::endl;
        }
        relativeError = fabsf(distance - expectedDistance) / startDistance;
        if (relativeError > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray capsule intersection distance error = " << relativeError << std::endl;
        }

        // toward top cap
        float delta = 2.0f * EPSILON;
        rayStart.y = halfHeight + delta;
        distance = FLT_MAX;
        if (!capsule.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit capsule" << std::endl;
        }
        relativeError = fabsf(distance - expectedDistance) / startDistance;
        if (relativeError > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray capsule intersection distance error = " << relativeError << std::endl;
        }

        const float EDGE_CASE_SLOP_FACTOR = 20.0f;

        // toward tip of top cap
        rayStart.y = halfHeight + radius - delta;
        distance = FLT_MAX;
        if (!capsule.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit capsule" << std::endl;
        }
        expectedDistance = startDistance - radius * sqrtf(2.0f * delta);    // using small angle approximation of cosine
        relativeError = fabsf(distance - expectedDistance) / startDistance;
        // for edge cases we allow a LOT of error
        if (relativeError > EDGE_CASE_SLOP_FACTOR * EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray capsule intersection distance error = " << relativeError << std::endl;
        }

        // toward tip of bottom cap
        rayStart.y = - halfHeight - radius + delta;
        distance = FLT_MAX;
        if (!capsule.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit capsule" << std::endl;
        }
        expectedDistance = startDistance - radius * sqrtf(2.0f * delta);    // using small angle approximation of cosine
        relativeError = fabsf(distance - expectedDistance) / startDistance;
        // for edge cases we allow a LOT of error
        if (relativeError > EDGE_CASE_SLOP_FACTOR * EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray capsule intersection distance error = " << relativeError << std::endl;
        }

        // toward edge of capsule cylindrical face
        rayStart.y = 0.0f;
        rayStart.z = radius - delta;
        distance = FLT_MAX;
        if (!capsule.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit capsule" << std::endl;
        }
        expectedDistance = startDistance - radius * sqrtf(2.0f * delta);    // using small angle approximation of cosine
        relativeError = fabsf(distance - expectedDistance) / startDistance;
        // for edge cases we allow a LOT of error
        if (relativeError > EDGE_CASE_SLOP_FACTOR * EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray capsule intersection distance error = " << relativeError << std::endl;
        }
    }
    // TODO: test at steep angles near cylinder/cap junction
}

void ShapeColliderTests::rayMissesCapsule() {
    // same as edge case hit tests, but shifted in the opposite direction
    float startDistance = 3.0f;
    float radius = 1.0f;
    float halfHeight = 2.0f;
    glm::vec3 center(0.0f);
    CapsuleShape capsule(radius, halfHeight);

    { // simple test along xAxis 
        // toward capsule center
        glm::vec3 rayStart(startDistance, 0.0f, 0.0f);
        glm::vec3 rayDirection(-1.0f, 0.0f, 0.0f);
        float delta = 2.0f * EPSILON;

        // over top cap
        rayStart.y = halfHeight + radius + delta;
        float distance = FLT_MAX;
        if (capsule.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss capsule" << std::endl;
        }
        if (distance != FLT_MAX) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss" << std::endl;
        }

        // below bottom cap
        rayStart.y = - halfHeight - radius - delta;
        distance = FLT_MAX;
        if (capsule.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss capsule" << std::endl;
        }
        if (distance != FLT_MAX) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss" << std::endl;
        }

        // past edge of capsule cylindrical face
        rayStart.y = 0.0f;
        rayStart.z = radius + delta;
        distance = FLT_MAX;
        if (capsule.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss capsule" << std::endl;
        }
        if (distance != FLT_MAX) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss" << std::endl;
        }
    }
    // TODO: test at steep angles near edge
}

void ShapeColliderTests::rayHitsPlane() {
    // make a simple plane
    float planeDistanceFromOrigin = 3.579f;
    glm::vec3 planePosition(0.0f, planeDistanceFromOrigin, 0.0f);
    PlaneShape plane;
    plane.setTranslation(planePosition);

    // make a simple ray
    float startDistance = 1.234f;
    glm::vec3 rayStart(-startDistance, 0.0f, 0.0f);
    glm::vec3 rayDirection = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));

    float distance = FLT_MAX;
    if (!plane.findRayIntersection(rayStart, rayDirection, distance)) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit plane" << std::endl;
    }

    float expectedDistance = SQUARE_ROOT_OF_3 * planeDistanceFromOrigin;
    float relativeError = fabsf(distance - expectedDistance) / planeDistanceFromOrigin;
    if (relativeError > EPSILON) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray plane intersection distance error = " << relativeError << std::endl;
    }

    // rotate the whole system and try again
    float angle = 37.8f;
    glm::vec3 axis = glm::normalize( glm::vec3(-7.0f, 2.8f, 9.3f) );
    glm::quat rotation = glm::angleAxis(angle, axis);

    plane.setTranslation(rotation * planePosition);
    plane.setRotation(rotation);
    rayStart = rotation * rayStart;
    rayDirection = rotation * rayDirection;

    distance = FLT_MAX;
    if (!plane.findRayIntersection(rayStart, rayDirection, distance)) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit plane" << std::endl;
    }

    expectedDistance = SQUARE_ROOT_OF_3 * planeDistanceFromOrigin;
    relativeError = fabsf(distance - expectedDistance) / planeDistanceFromOrigin;
    if (relativeError > EPSILON) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray plane intersection distance error = " << relativeError << std::endl;
    }
}

void ShapeColliderTests::rayMissesPlane() {
    // make a simple plane
    float planeDistanceFromOrigin = 3.579f;
    glm::vec3 planePosition(0.0f, planeDistanceFromOrigin, 0.0f);
    PlaneShape plane;
    plane.setTranslation(planePosition);

    { // parallel rays should miss
        float startDistance = 1.234f;
        glm::vec3 rayStart(-startDistance, 0.0f, 0.0f);
        glm::vec3 rayDirection = glm::normalize(glm::vec3(-1.0f, 0.0f, -1.0f));
    
        float distance = FLT_MAX;
        if (plane.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss plane" << std::endl;
        }
        if (distance != FLT_MAX) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss" << std::endl;
        }
    
        // rotate the whole system and try again
        float angle = 37.8f;
        glm::vec3 axis = glm::normalize( glm::vec3(-7.0f, 2.8f, 9.3f) );
        glm::quat rotation = glm::angleAxis(angle, axis);
    
        plane.setTranslation(rotation * planePosition);
        plane.setRotation(rotation);
        rayStart = rotation * rayStart;
        rayDirection = rotation * rayDirection;
    
        distance = FLT_MAX;
        if (plane.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss plane" << std::endl;
        }
        if (distance != FLT_MAX) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss" << std::endl;
        }
    }

    { // make a simple ray that points away from plane
        float startDistance = 1.234f;
        glm::vec3 rayStart(-startDistance, 0.0f, 0.0f);
        glm::vec3 rayDirection = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
    
        float distance = FLT_MAX;
        if (plane.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss plane" << std::endl;
        }
        if (distance != FLT_MAX) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss" << std::endl;
        }
    
        // rotate the whole system and try again
        float angle = 37.8f;
        glm::vec3 axis = glm::normalize( glm::vec3(-7.0f, 2.8f, 9.3f) );
        glm::quat rotation = glm::angleAxis(angle, axis);
    
        plane.setTranslation(rotation * planePosition);
        plane.setRotation(rotation);
        rayStart = rotation * rayStart;
        rayDirection = rotation * rayDirection;
    
        distance = FLT_MAX;
        if (plane.findRayIntersection(rayStart, rayDirection, distance)) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss plane" << std::endl;
        }
        if (distance != FLT_MAX) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss" << std::endl;
        }
    }
}

void ShapeColliderTests::measureTimeOfCollisionDispatch() {
    /* KEEP for future manual testing
    // create two non-colliding spheres
    float radiusA = 7.0f;
    float radiusB = 3.0f;
    float alpha = 1.2f;
    float beta = 1.3f;
    glm::vec3 offsetDirection = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
    float offsetDistance = alpha * radiusA + beta * radiusB;

    SphereShape sphereA(radiusA, origin);
    SphereShape sphereB(radiusB, offsetDistance * offsetDirection);
    CollisionList collisions(16);

    //int numTests = 1;
    quint64 oldTime;
    quint64 newTime;
    int numTests = 100000000;
    {
        quint64 startTime = usecTimestampNow();
        for (int i = 0; i < numTests; ++i) {
            ShapeCollider::collideShapes(&sphereA, &sphereB, collisions);
        }
        quint64 endTime = usecTimestampNow();
        std::cout << numTests << " non-colliding collisions in " << (endTime - startTime) << " usec" << std::endl;
        newTime = endTime - startTime;
    }
    */
}

void ShapeColliderTests::runAllTests() {
    ShapeCollider::initDispatchTable();

    //measureTimeOfCollisionDispatch();

    sphereMissesSphere();
    sphereTouchesSphere();

    sphereMissesCapsule();
    sphereTouchesCapsule();

    capsuleMissesCapsule();
    capsuleTouchesCapsule();

    sphereTouchesAACubeFaces();
    sphereTouchesAACubeEdges();
    sphereMissesAACube();

    rayHitsSphere();
    rayBarelyHitsSphere();
    rayBarelyMissesSphere();
    rayHitsCapsule();
    rayMissesCapsule();
    rayHitsPlane();
    rayMissesPlane();
}
