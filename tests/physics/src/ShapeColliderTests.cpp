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

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <CollisionInfo.h>
#include <ShapeCollider.h>
#include <SharedUtil.h>
#include <SphereShape.h>
#include <StreamUtils.h>

#include "ShapeColliderTests.h"

const glm::vec3 origin(0.f);
static const glm::vec3 xAxis(1.f, 0.f, 0.f);
static const glm::vec3 yAxis(0.f, 1.f, 0.f);
static const glm::vec3 zAxis(0.f, 0.f, 1.f);

void ShapeColliderTests::sphereMissesSphere() {
    // non-overlapping spheres of unequal size
    float radiusA = 7.f;
    float radiusB = 3.f;
    float alpha = 1.2f;
    float beta = 1.3f;
    glm::vec3 offsetDirection = glm::normalize(glm::vec3(1.f, 2.f, 3.f));
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
    float radiusA = 7.f;
    float radiusB = 3.f;
    float alpha = 0.2f;
    float beta = 0.3f;
    glm::vec3 offsetDirection = glm::normalize(glm::vec3(1.f, 2.f, 3.f));
    float offsetDistance = alpha * radiusA + beta * radiusB;
    float expectedPenetrationDistance = (1.f - alpha) * radiusA + (1.f - beta) * radiusB;
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
        }
    
        // penetration points from sphereA into sphereB
        float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration 
                << std::endl;
        }
    
        // contactPoint is on surface of sphereA
        glm::vec3 AtoB = sphereB.getPosition() - sphereA.getPosition();
        glm::vec3 expectedContactPoint = sphereA.getPosition() + radiusA * glm::normalize(AtoB);
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint 
                << std::endl;
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
                << " actual = " << collision->_penetration 
                << std::endl;
        }
    
        // contactPoint is on surface of sphereA
        glm::vec3 BtoA = sphereA.getPosition() - sphereB.getPosition();
        glm::vec3 expectedContactPoint = sphereB.getPosition() + radiusB * glm::normalize(BtoA);
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint 
                << std::endl;
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
    glm::vec3 axis = glm::normalize( glm::vec3(-7.f, 2.8f, 9.3f) );
    glm::quat rotation = glm::angleAxis(angle, axis);
    glm::vec3 translation(15.1f, -27.1f, -38.6f);
    capsuleB.setRotation(rotation);
    capsuleB.setPosition(translation);

    CollisionList collisions(16);

    // walk sphereA along the local yAxis next to, but not touching, capsuleB
    glm::vec3 localStartPosition(radialOffset, axialOffset, 0.f);
    int numberOfSteps = 10;
    float delta = 1.3f * (totalRadius + halfHeightB) / (numberOfSteps - 1);
    for (int i = 0; i < numberOfSteps; ++i) {
        // translate sphereA into world-frame
        glm::vec3 localPosition = localStartPosition + ((float)i * delta) * yAxis;
        sphereA.setPosition(rotation * localPosition + translation);

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
    float radiusA = 2.f;
    float radiusB = 1.f;
    float totalRadius = radiusA + radiusB;
    float halfHeightB = 2.f;
    float alpha = 0.5f;
    float beta = 0.5f;
    float radialOffset = alpha * radiusA + beta * radiusB;
    
    SphereShape sphereA(radiusA);
    CapsuleShape capsuleB(radiusB, halfHeightB);

    CollisionList collisions(16);
    int numCollisions = 0;

    {   // sphereA collides with capsuleB's cylindrical wall
        sphereA.setPosition(radialOffset * xAxis);

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
                << " actual = " << collision->_penetration 
                << std::endl;
        }
    
        // contactPoint is on surface of sphereA
        glm::vec3 expectedContactPoint = sphereA.getPosition() - radiusA * xAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint 
                << std::endl;
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
                << " actual = " << collision->_penetration 
                << std::endl;
        }
    
        // contactPoint is on surface of capsuleB
        glm::vec3 BtoA = sphereA.getPosition() - capsuleB.getPosition();
        glm::vec3 closestApproach = capsuleB.getPosition() + glm::dot(BtoA, yAxis) * yAxis;
        expectedContactPoint = closestApproach + radiusB * glm::normalize(BtoA - closestApproach);
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint 
                << std::endl;
        }
    }
    {   // sphereA hits end cap at axis
        glm::vec3 axialOffset = (halfHeightB + alpha * radiusA + beta * radiusB) * yAxis;
        sphereA.setPosition(axialOffset * yAxis);
        
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
        glm::vec3 expectedPenetration = - ((1.f - alpha) * radiusA + (1.f - beta) * radiusB) * yAxis;
        float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration 
                << std::endl;
        }
    
        // contactPoint is on surface of sphereA
        glm::vec3 expectedContactPoint = sphereA.getPosition() - radiusA * yAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint 
                << std::endl;
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
        expectedPenetration = ((1.f - alpha) * radiusA + (1.f - beta) * radiusB) * yAxis;
        inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration 
                << std::endl;
        }
    
        // contactPoint is on surface of capsuleB
        glm::vec3 endPoint;
        capsuleB.getEndPoint(endPoint);
        expectedContactPoint = endPoint + radiusB * yAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint 
                << std::endl;
        }
    }
    {   // sphereA hits start cap at axis
        glm::vec3 axialOffset = - (halfHeightB + alpha * radiusA + beta * radiusB) * yAxis;
        sphereA.setPosition(axialOffset * yAxis);
        
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
        glm::vec3 expectedPenetration = ((1.f - alpha) * radiusA + (1.f - beta) * radiusB) * yAxis;
        float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration 
                << std::endl;
        }
    
        // contactPoint is on surface of sphereA
        glm::vec3 expectedContactPoint = sphereA.getPosition() + radiusA * yAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint 
                << std::endl;
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
        expectedPenetration = - ((1.f - alpha) * radiusA + (1.f - beta) * radiusB) * yAxis;
        inaccuracy = glm::length(collision->_penetration - expectedPenetration);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad penetration: expected = " << expectedPenetration
                << " actual = " << collision->_penetration 
                << std::endl;
        }
    
        // contactPoint is on surface of capsuleB
        glm::vec3 startPoint;
        capsuleB.getStartPoint(startPoint);
        expectedContactPoint = startPoint - radiusB * yAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint 
                << std::endl;
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
    float radiusA = 2.f;
    float halfHeightA = 3.f;
    float radiusB = 3.f;
    float halfHeightB = 4.f;

    float totalRadius = radiusA + radiusB;
    float totalHalfLength = totalRadius + halfHeightA + halfHeightB;

    CapsuleShape capsuleA(radiusA, halfHeightA);
    CapsuleShape capsuleB(radiusA, halfHeightA);

    CollisionList collisions(16);

    // side by side
    capsuleB.setPosition((1.01f * totalRadius) * xAxis);
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
    capsuleB.setPosition((1.01f * totalHalfLength) * xAxis);
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
    capsuleB.setPosition((1.01f * (totalRadius + capsuleB.getHalfHeight())) * xAxis);
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
    float radiusA = 2.f;
    float halfHeightA = 3.f;
    float radiusB = 3.f;
    float halfHeightB = 4.f;

    float totalRadius = radiusA + radiusB;
    float totalHalfLength = totalRadius + halfHeightA + halfHeightB;

    CapsuleShape capsuleA(radiusA, halfHeightA);
    CapsuleShape capsuleB(radiusB, halfHeightB);

    CollisionList collisions(16);
    int numCollisions = 0;

    { // side by side
        capsuleB.setPosition((0.99f * totalRadius) * xAxis);
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
        capsuleB.setPosition((0.99f * totalHalfLength) * yAxis);

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
        capsuleB.setPosition((0.99f * (totalRadius + capsuleB.getHalfHeight())) * xAxis);

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
        capsuleB.setPosition(positionB);

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
                << " actual = " << collision->_penetration 
                << std::endl;
        }
    
        glm::vec3 expectedContactPoint = capsuleA.getPosition() + radiusA * xAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint 
                << std::endl;
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
    
        expectedContactPoint = capsuleB.getPosition() - (radiusB + halfHeightB) * xAxis;
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
        capsuleB.setPosition(positionB);

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
    
        glm::vec3 expectedContactPoint = capsuleA.getPosition() + radiusA * zAxis + shift * yAxis;
        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
        if (fabs(inaccuracy) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
                << " actual = " << collision->_contactPoint 
                << std::endl;
        }
    }
}


void ShapeColliderTests::runAllTests() {
    sphereMissesSphere();
    sphereTouchesSphere();

    sphereMissesCapsule();
    sphereTouchesCapsule();

    capsuleMissesCapsule();
    capsuleTouchesCapsule();
}
