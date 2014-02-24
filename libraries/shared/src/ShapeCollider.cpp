//
//  ShapeCollider.cpp
//  hifi
//
//  Created by Andrew Meadows on 2014.02.20
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <iostream>

#include <glm/gtx/norm.hpp>

#include "ShapeCollider.h"

namespace ShapeCollider {

bool shapeShape(const Shape* shapeA, const Shape* shapeB, CollisionInfo& collision) {
    // ATM we only have two shape types so we just check every case.
    // TODO: make a fast lookup for correct method
    if (shapeA->getType() == Shape::SPHERE_SHAPE) {
        const SphereShape* sphereA = static_cast<const SphereShape*>(shapeA);
        if (shapeB->getType() == Shape::SPHERE_SHAPE) {
            return sphereSphere(sphereA, static_cast<const SphereShape*>(shapeB), collision);
        } else if (shapeB->getType() == Shape::CAPSULE_SHAPE) {
            return sphereCapsule(sphereA, static_cast<const CapsuleShape*>(shapeB), collision);
        }
    } else if (shapeA->getType() == Shape::CAPSULE_SHAPE) {
        const CapsuleShape* capsuleA = static_cast<const CapsuleShape*>(shapeA);
        if (shapeB->getType() == Shape::SPHERE_SHAPE) {
            return capsuleSphere(capsuleA, static_cast<const SphereShape*>(shapeB), collision);
        } else if (shapeB->getType() == Shape::CAPSULE_SHAPE) {
            return capsuleCapsule(capsuleA, static_cast<const CapsuleShape*>(shapeB), collision);
        }
    }
    return false;
}

bool sphereSphere(const SphereShape* sphereA, const SphereShape* sphereB, CollisionInfo& collision) {
    glm::vec3 BA = sphereB->getPosition() - sphereA->getPosition();
    float distanceSquared = glm::dot(BA, BA);
    float totalRadius = sphereA->getRadius() + sphereB->getRadius();
    if (distanceSquared < totalRadius * totalRadius) {
        // normalize BA
        float distance = sqrtf(distanceSquared);
        if (distanceSquared < EPSILON) {
            // the spheres are on top of each other, so we pick an arbitrary penetration direction
            BA = glm::vec3(0.f, 1.f, 0.f);
        } else {
            BA /= distance;
        }
        // penetration points from A into B
        collision._penetration = BA * (totalRadius - distance);
        // contactPoint is on surface of A
        collision._contactPoint = sphereA->getPosition() + sphereA->getRadius() * BA;
        return true;
    }
    return false;
}

bool sphereCapsule(const SphereShape* sphereA, const CapsuleShape* capsuleB, CollisionInfo& collision) {
    // find sphereA's closest approach to axis of capsuleB
    glm::vec3 BA = capsuleB->getPosition() - sphereA->getPosition();
    glm::vec3 capsuleAxis; 
    capsuleB->computeNormalizedAxis(capsuleAxis);
    float axialDistance = - glm::dot(BA, capsuleAxis);
    float absAxialDistance = fabs(axialDistance);
    float totalRadius = sphereA->getRadius() + capsuleB->getRadius();
    if (absAxialDistance < totalRadius + capsuleB->getHalfHeight()) {
        glm::vec3 radialAxis = BA + axialDistance * capsuleAxis; // points from A to axis of B
        float radialDistance2 = glm::length2(radialAxis);
        if (radialDistance2 > totalRadius * totalRadius) {
            // sphere is too far from capsule axis
            return false;
        }
        if (absAxialDistance > capsuleB->getHalfHeight()) {
            // sphere hits capsule on a cap --> recompute radialAxis to point from spherA to cap center
            float sign = (axialDistance > 0.f) ? 1.f : -1.f;
            radialAxis = BA + (sign * capsuleB->getHalfHeight()) * capsuleAxis;
            radialDistance2 = glm::length2(radialAxis);
        }
        if (radialDistance2 > EPSILON * EPSILON) {
            // normalize the radialAxis
            float radialDistance = sqrtf(radialDistance2);
            radialAxis /= radialDistance;
            // penetration points from A into B
            collision._penetration = (totalRadius - radialDistance) * radialAxis; // points from A into B
            // contactPoint is on surface of sphereA
            collision._contactPoint = sphereA->getPosition() + sphereA->getRadius() * radialAxis;
        } else {
            // A is on B's axis, so the penetration is undefined... 
            if (absAxialDistance > capsuleB->getHalfHeight()) {
                // ...for the cylinder case (for now we pretend the collision doesn't exist)
                return false;
            }
            // ... but still defined for the cap case
            if (axialDistance < 0.f) {
                // we're hitting the start cap, so we negate the capsuleAxis
                capsuleAxis *= -1;
            }
            // penetration points from A into B
            float sign = (axialDistance > 0.f) ? -1.f : 1.f;
            collision._penetration = (sign * (totalRadius + capsuleB->getHalfHeight() - absAxialDistance)) * capsuleAxis;
            // contactPoint is on surface of sphereA
            collision._contactPoint = sphereA->getPosition() + (sign * sphereA->getRadius()) * capsuleAxis;
        }
        return true;
    }
    return false;
}

bool capsuleSphere(const CapsuleShape* capsuleA, const SphereShape* sphereB, CollisionInfo& collision) {
    // find sphereB's closest approach to axis of capsuleA
    glm::vec3 AB = capsuleA->getPosition() - sphereB->getPosition();
    glm::vec3 capsuleAxis;
    capsuleA->computeNormalizedAxis(capsuleAxis);
    float axialDistance = - glm::dot(AB, capsuleAxis);
    float absAxialDistance = fabs(axialDistance);
    float totalRadius = sphereB->getRadius() + capsuleA->getRadius();
    if (absAxialDistance < totalRadius + capsuleA->getHalfHeight()) {
        glm::vec3 radialAxis = AB + axialDistance * capsuleAxis; // from sphereB to axis of capsuleA
        float radialDistance2 = glm::length2(radialAxis);
        if (radialDistance2 > totalRadius * totalRadius) {
            // sphere is too far from capsule axis
            return false;
        }

        // closestApproach = point on capsuleA's axis that is closest to sphereB's center
        glm::vec3 closestApproach = capsuleA->getPosition() + axialDistance * capsuleAxis;

        if (absAxialDistance > capsuleA->getHalfHeight()) {
            // sphere hits capsule on a cap 
            // --> recompute radialAxis and closestApproach
            float sign = (axialDistance > 0.f) ? 1.f : -1.f;
            closestApproach = capsuleA->getPosition() + (sign * capsuleA->getHalfHeight()) * capsuleAxis;
            radialAxis = closestApproach - sphereB->getPosition();
            radialDistance2 = glm::length2(radialAxis);
        }
        if (radialDistance2 > EPSILON * EPSILON) {
            // normalize the radialAxis
            float radialDistance = sqrtf(radialDistance2);
            radialAxis /= radialDistance;
            // penetration points from A into B
            collision._penetration = (radialDistance - totalRadius) * radialAxis; // points from A into B
            // contactPoint is on surface of capsuleA
            collision._contactPoint = closestApproach - capsuleA->getRadius() * radialAxis;
        } else {
            // A is on B's axis, so the penetration is undefined... 
            if (absAxialDistance > capsuleA->getHalfHeight()) {
                // ...for the cylinder case (for now we pretend the collision doesn't exist)
                return false;
            } else {
                // ... but still defined for the cap case
                if (axialDistance < 0.f) {
                    // we're hitting the start cap, so we negate the capsuleAxis
                    capsuleAxis *= -1;
                }
                float sign = (axialDistance > 0.f) ? 1.f : -1.f;
                collision._penetration = (sign * (totalRadius + capsuleA->getHalfHeight() - absAxialDistance)) * capsuleAxis;
                // contactPoint is on surface of sphereA
                collision._contactPoint = closestApproach + (sign * capsuleA->getRadius()) * capsuleAxis;
            }
        }
        return true;
    }
    return false;
}

bool capsuleCapsule(const CapsuleShape* capsuleA, const CapsuleShape* capsuleB, CollisionInfo& collision) {
    return false;
}

}   // namespace ShapeCollider
