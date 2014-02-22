//
//  ShapeCollider.cpp
//  hifi
//
//  Created by Andrew Meadows on 2014.02.20
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "ShapeCollider.h"

namespace ShapeCollider {

bool shapeShape(const Shape* shapeA, const Shape* shapeB, 
        const glm::quat& rotationAB, const glm::vec3& offsetB, CollisionInfo& collision) {
    // ATM we only have two shape types so we just check every case.
    // TODO: make a fast lookup for correct method
    if (shapeA->getType() == Shape::SPHERE_SHAPE) {
        const SphereShape* sphereA = static_cast<const SphereShape*>(shapeA);
        if (shapeB->getType() == Shape::SPHERE_SHAPE) {
            return sphereSphere(sphereA, static_cast<const SphereShape*>(shapeB), 
                    rotationAB, offsetB, collision);
        } else if (shapeB->getType() == Shape::CAPSULE_SHAPE) {
            return sphereCapsule(sphereA, static_cast<const CapsuleShape*>(shapeB), 
                    rotationAB, offsetB, collision);
        }
    } else if (shapeA->getType() == Shape::CAPSULE_SHAPE) {
        const CapsuleShape* capsuleA = static_cast<const CapsuleShape*>(shapeA);
        if (shapeB->getType() == Shape::SPHERE_SHAPE) {
            return capsuleSphere(capsuleA, static_cast<const SphereShape*>(shapeB), 
                    rotationAB, offsetB, collision);
        } else if (shapeB->getType() == Shape::CAPSULE_SHAPE) {
            return capsuleCapsule(capsuleA, static_cast<const CapsuleShape*>(shapeB), 
                    rotationAB, offsetB, collision);
        }
    }
    return false;
}

bool sphereSphere(const SphereShape* sphereA, const SphereShape* sphereB, 
        const glm::quat& rotationAB, const glm::vec3& offsetB, CollisionInfo& collision) {
    // A in B's frame
    glm::vec3 A = offsetB + rotationAB * sphereA->getPosition();
    // BA = B - A = from A to B, in B's frame
    glm::vec3 BA = sphereB->getPosition() - A;
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
        // store the collision details in B's frame
        collision._penetration = BA * (totalRadius - distance);
        collision._contactPoint = A + sphereA->getRadius() * BA;
        return true;
    }
    return false;
}

// everything in the capsule's natural frame (where its axis is along yAxis)
bool sphereCapsuleHelper(float sphereRadius, const glm::vec3 sphereCenter, 
        const CapsuleShape* capsule, CollisionInfo& collision) {
    float xzSquared = sphereCenter.x * sphereCenter.x + sphereCenter.y * sphereCenter.y;
    float totalRadius = sphereRadius + capsule->getRadius();
    if (xzSquared < totalRadius * totalRadius) {
        float fabsY = fabs(sphereCenter.y);
        float halfHeight = capsule->getHalfHeight();
        if (fabsY < halfHeight) {
            // sphere collides with cylindrical wall
            glm::vec3 BA = -sphereCenter;
            BA.y = 0.f;
            float distance = sqrtf(xzSquared);
            if (distance < EPSILON) {
                // for now we don't handle this singular case
                return false;
            }
            BA /= distance;
            collision._penetration = BA * (totalRadius - distance);
            collision._contactPoint = BA * (sphereRadius - totalRadius + distance);
            collision._contactPoint.y = fabsY;
            if (sphereCenter.y < 0.f) {
                // negate the y elements of the collision info
                collision._penetration.y *= -1.f;
                collision._contactPoint.y *= -1.f;
            }
            return true;
        } else {
            // tansform into frame where cap is at origin
            float newY = fabsY - halfHeight;
            float distance = sqrtf(newY * newY + xzSquared);
            if (distance < totalRadius) {
                // sphere collides with cap

                // BA points from sphere to cap
                glm::vec3 BA(-sphereCenter.x, newY, -sphereCenter.z);
                if (distance < EPSILON) {
                    // for now we don't handle this singular case
                    return false;
                }
                BA /= distance;
                collision._penetration = BA * (totalRadius - distance);
                collision._contactPoint = BA * (sphereRadius - totalRadius + distance);
                collision._contactPoint.y += halfHeight;

                if (sphereCenter.y < 0.f) {
                    // negate the y elements of the collision info
                    collision._penetration.y *= -1.f;
                    collision._contactPoint.y *= -1.f;
                }
                return true;
            }
        }
    }
    return false;
}

bool sphereCapsule(const SphereShape* sphereA, const CapsuleShape* capsuleB,
        const glm::quat& rotationAB, const glm::vec3& offsetB, CollisionInfo& collision) {
    // transform sphereA all the way to capsuleB's natural frame
    glm::quat rotationB = capsuleB->getRotation();
    glm::vec3 sphereCenter = rotationB * (offsetB + rotationAB * sphereA->getPosition() - capsuleB->getPosition());
    if (sphereCapsuleHelper(sphereA->getRadius(), sphereCenter, capsuleB, collision)) {
        // need to transform collision details back into B's offset frame
        collision.rotateThenTranslate(glm::inverse(rotationB), capsuleB->getPosition());
        return true;
    }
    return false;
}

bool capsuleSphere(const CapsuleShape* capsuleA, const SphereShape* sphereB,
        const glm::quat& rotationAB, const glm::vec3& offsetB, CollisionInfo& collision) {
    // transform sphereB all the way to capsuleA's natural frame
    glm::quat rotationBA = glm::inverse(rotationAB);
    glm::quat rotationA = capsuleA->getRotation();
    glm::vec3 offsetA = rotationBA * (-offsetB);
    glm::vec3 sphereCenter = rotationA * (offsetA + rotationBA * sphereB->getPosition() - capsuleA->getPosition());
    if (sphereCapsuleHelper(sphereB->getRadius(), sphereCenter, capsuleA, collision)) {
        // need to transform collision details back into B's offset frame
        // BUG: these back translations are probably not right
        collision.rotateThenTranslate(glm::inverse(rotationA), capsuleA->getPosition());
        collision.rotateThenTranslate(glm::inverse(rotationAB), -offsetB);
        return true;
    }
    return false;
}

bool capsuleCapsule(const CapsuleShape* capsuleA, const CapsuleShape* capsuleB,
        const glm::quat& rotationAB, const glm::vec3& offsetB, CollisionInfo& collision) {
    return false;
}

}   // namespace ShapeCollider
