//
//  ShapeCollider.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows on 02/20/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>

#include <glm/gtx/norm.hpp>

#include "ShapeCollider.h"

// NOTE:
//
// * Large ListShape's are inefficient keep the lists short.
// * Collisions between lists of lists work in theory but are not recommended.

namespace ShapeCollider {

bool shapeShape(const Shape* shapeA, const Shape* shapeB, CollisionList& collisions) {
    // ATM we only have two shape types so we just check every case.
    // TODO: make a fast lookup for correct method
    int typeA = shapeA->getType();
    int typeB = shapeB->getType();
    if (typeA == Shape::SPHERE_SHAPE) {
        const SphereShape* sphereA = static_cast<const SphereShape*>(shapeA);
        if (typeB == Shape::SPHERE_SHAPE) {
            return sphereSphere(sphereA, static_cast<const SphereShape*>(shapeB), collisions);
        } else if (typeB == Shape::CAPSULE_SHAPE) {
            return sphereCapsule(sphereA, static_cast<const CapsuleShape*>(shapeB), collisions);
        }
    } else if (typeA == Shape::CAPSULE_SHAPE) {
        const CapsuleShape* capsuleA = static_cast<const CapsuleShape*>(shapeA);
        if (typeB == Shape::SPHERE_SHAPE) {
            return capsuleSphere(capsuleA, static_cast<const SphereShape*>(shapeB), collisions);
        } else if (typeB == Shape::CAPSULE_SHAPE) {
            return capsuleCapsule(capsuleA, static_cast<const CapsuleShape*>(shapeB), collisions);
        }
    } else if (typeA == Shape::LIST_SHAPE) {
        const ListShape* listA = static_cast<const ListShape*>(shapeA);
        if (typeB == Shape::SPHERE_SHAPE) {
            return listSphere(listA, static_cast<const SphereShape*>(shapeB), collisions);
        } else if (typeB == Shape::CAPSULE_SHAPE) {
            return listCapsule(listA, static_cast<const CapsuleShape*>(shapeB), collisions);
        }
    }
    return false;
}

bool sphereSphere(const SphereShape* sphereA, const SphereShape* sphereB, CollisionList& collisions) {
    glm::vec3 BA = sphereB->getPosition() - sphereA->getPosition();
    float distanceSquared = glm::dot(BA, BA);
    float totalRadius = sphereA->getRadius() + sphereB->getRadius();
    if (distanceSquared < totalRadius * totalRadius) {
        // normalize BA
        float distance = sqrtf(distanceSquared);
        if (distance < EPSILON) {
            // the spheres are on top of each other, so we pick an arbitrary penetration direction
            BA = glm::vec3(0.f, 1.f, 0.f);
            distance = totalRadius;
        } else {
            BA /= distance;
        }
        // penetration points from A into B
        CollisionInfo* collision = collisions.getNewCollision();
        if (collision) {
            collision->_penetration = BA * (totalRadius - distance);
            // contactPoint is on surface of A
            collision->_contactPoint = sphereA->getPosition() + sphereA->getRadius() * BA;
            return true;
        }
    }
    return false;
}

bool sphereCapsule(const SphereShape* sphereA, const CapsuleShape* capsuleB, CollisionList& collisions) {
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
        float totalRadius2 = totalRadius * totalRadius;
        if (radialDistance2 > totalRadius2) {
            // sphere is too far from capsule axis
            return false;
        }
        if (absAxialDistance > capsuleB->getHalfHeight()) {
            // sphere hits capsule on a cap --> recompute radialAxis to point from spherA to cap center
            float sign = (axialDistance > 0.f) ? 1.f : -1.f;
            radialAxis = BA + (sign * capsuleB->getHalfHeight()) * capsuleAxis;
            radialDistance2 = glm::length2(radialAxis);
            if (radialDistance2 > totalRadius2) {
                return false;
            }
        }
        if (radialDistance2 > EPSILON * EPSILON) {
            CollisionInfo* collision = collisions.getNewCollision();
            if (!collision) {
                // collisions list is full
                return false;
            }
            // normalize the radialAxis
            float radialDistance = sqrtf(radialDistance2);
            radialAxis /= radialDistance;
            // penetration points from A into B
            collision->_penetration = (totalRadius - radialDistance) * radialAxis; // points from A into B
            // contactPoint is on surface of sphereA
            collision->_contactPoint = sphereA->getPosition() + sphereA->getRadius() * radialAxis;
        } else {
            // A is on B's axis, so the penetration is undefined... 
            if (absAxialDistance > capsuleB->getHalfHeight()) {
                // ...for the cylinder case (for now we pretend the collision doesn't exist)
                return false;
            }
            CollisionInfo* collision = collisions.getNewCollision();
            if (!collision) {
                // collisions list is full
                return false;
            }
            // ... but still defined for the cap case
            if (axialDistance < 0.f) {
                // we're hitting the start cap, so we negate the capsuleAxis
                capsuleAxis *= -1;
            }
            // penetration points from A into B
            float sign = (axialDistance > 0.f) ? -1.f : 1.f;
            collision->_penetration = (sign * (totalRadius + capsuleB->getHalfHeight() - absAxialDistance)) * capsuleAxis;
            // contactPoint is on surface of sphereA
            collision->_contactPoint = sphereA->getPosition() + (sign * sphereA->getRadius()) * capsuleAxis;
        }
        return true;
    }
    return false;
}

bool capsuleSphere(const CapsuleShape* capsuleA, const SphereShape* sphereB, CollisionList& collisions) {
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
        float totalRadius2 = totalRadius * totalRadius;
        if (radialDistance2 > totalRadius2) {
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
            if (radialDistance2 > totalRadius2) {
                return false;
            }
        }
        if (radialDistance2 > EPSILON * EPSILON) {
            CollisionInfo* collision = collisions.getNewCollision();
            if (!collision) {
                // collisions list is full
                return false;
            }
            // normalize the radialAxis
            float radialDistance = sqrtf(radialDistance2);
            radialAxis /= radialDistance;
            // penetration points from A into B
            collision->_penetration = (radialDistance - totalRadius) * radialAxis; // points from A into B
            // contactPoint is on surface of capsuleA
            collision->_contactPoint = closestApproach - capsuleA->getRadius() * radialAxis;
        } else {
            // A is on B's axis, so the penetration is undefined... 
            if (absAxialDistance > capsuleA->getHalfHeight()) {
                // ...for the cylinder case (for now we pretend the collision doesn't exist)
                return false;
            } else {
                CollisionInfo* collision = collisions.getNewCollision();
                if (!collision) {
                    // collisions list is full
                    return false;
                }
                // ... but still defined for the cap case
                if (axialDistance < 0.f) {
                    // we're hitting the start cap, so we negate the capsuleAxis
                    capsuleAxis *= -1;
                }
                float sign = (axialDistance > 0.f) ? 1.f : -1.f;
                collision->_penetration = (sign * (totalRadius + capsuleA->getHalfHeight() - absAxialDistance)) * capsuleAxis;
                // contactPoint is on surface of sphereA
                collision->_contactPoint = closestApproach + (sign * capsuleA->getRadius()) * capsuleAxis;
            }
        }
        return true;
    }
    return false;
}

bool capsuleCapsule(const CapsuleShape* capsuleA, const CapsuleShape* capsuleB, CollisionList& collisions) {
    glm::vec3 axisA;
    capsuleA->computeNormalizedAxis(axisA);
    glm::vec3 axisB;
    capsuleB->computeNormalizedAxis(axisB);
    glm::vec3 centerA = capsuleA->getPosition();
    glm::vec3 centerB = capsuleB->getPosition();

    // NOTE: The formula for closest approach between two lines is:
    // d = [(B - A) . (a - (a.b)b)] / (1 - (a.b)^2)

    float aDotB = glm::dot(axisA, axisB);
    float denominator = 1.f - aDotB * aDotB;
    float totalRadius = capsuleA->getRadius() + capsuleB->getRadius();
    if (denominator > EPSILON) {
        // distances to points of closest approach
        float distanceA = glm::dot((centerB - centerA), (axisA - (aDotB) * axisB)) / denominator;
        float distanceB = glm::dot((centerA - centerB), (axisB - (aDotB) * axisA)) / denominator;
        
        // clamp the distances to the ends of the capsule line segments
        float absDistanceA = fabs(distanceA);
        if (absDistanceA > capsuleA->getHalfHeight() + capsuleA->getRadius()) {
            float signA = distanceA < 0.f ? -1.f : 1.f;
            distanceA = signA * capsuleA->getHalfHeight();
        }
        float absDistanceB = fabs(distanceB);
        if (absDistanceB > capsuleB->getHalfHeight() + capsuleB->getRadius()) {
            float signB = distanceB < 0.f ? -1.f : 1.f;
            distanceB = signB * capsuleB->getHalfHeight();
        }

        // collide like spheres at closest approaches (do most of the math relative to B)
        glm::vec3 BA = (centerB + distanceB * axisB) - (centerA + distanceA * axisA);
        float distanceSquared = glm::dot(BA, BA);
        if (distanceSquared < totalRadius * totalRadius) {
            CollisionInfo* collision = collisions.getNewCollision();
            if (!collision) {
                // collisions list is full
                return false;
            }
            // normalize BA
            float distance = sqrtf(distanceSquared);
            if (distance < EPSILON) {
                // the contact spheres are on top of each other, so we need to pick a penetration direction...
                // try vector between the capsule centers...
                BA = centerB - centerA;
                distanceSquared = glm::length2(BA);
                if (distanceSquared > EPSILON * EPSILON) {
                    distance = sqrtf(distanceSquared);
                    BA /= distance;
                } else
                {
                    // the capsule centers are on top of each other!
                    // give up on a valid penetration direction and just use the yAxis
                    BA = glm::vec3(0.f, 1.f, 0.f);
                    distance = glm::max(capsuleB->getRadius(), capsuleA->getRadius());
                }
            } else {
                BA /= distance;
            }
            // penetration points from A into B
            collision->_penetration = BA * (totalRadius - distance);
            // contactPoint is on surface of A
            collision->_contactPoint = centerA + distanceA * axisA + capsuleA->getRadius() * BA;
            return true;
        }
    } else {
        // capsules are approximiately parallel but might still collide
        glm::vec3 BA = centerB - centerA;
        float axialDistance = glm::dot(BA, axisB);
        if (axialDistance > totalRadius + capsuleA->getHalfHeight() + capsuleB->getHalfHeight()) {
            return false;
        }
        BA = BA - axialDistance * axisB;     // BA now points from centerA to axisB (perp to axis)
        float distanceSquared = glm::length2(BA);
        if (distanceSquared < totalRadius * totalRadius) {
            CollisionInfo* collision = collisions.getNewCollision();
            if (!collision) {
                // collisions list is full
                return false;
            }
            // We have all the info we need to compute the penetration vector...
            // normalize BA
            float distance = sqrtf(distanceSquared);
            if (distance < EPSILON) {
                // the spheres are on top of each other, so we pick an arbitrary penetration direction
                BA = glm::vec3(0.f, 1.f, 0.f);
            } else {
                BA /= distance;
            }
            // penetration points from A into B
            collision->_penetration = BA * (totalRadius - distance);

            // However we need some more world-frame info to compute the contactPoint, 
            // which is on the surface of capsuleA...
            //
            // Find the overlapping secion of the capsules --> they collide as if there were
            // two spheres at the midpoint of this overlapping section.  
            // So we project all endpoints to axisB, find the interior pair, 
            // and put A's proxy sphere on axisA at the midpoint of this section.

            // sort the projections as much as possible during calculation
            float points[5];
            points[0] = -capsuleB->getHalfHeight();
            points[1] = axialDistance - capsuleA->getHalfHeight();
            points[2] = axialDistance + capsuleA->getHalfHeight();
            points[3] = capsuleB->getHalfHeight();

            // Since there are only three comparisons to do we unroll the sort algorithm...
            // and use a fifth slot as temp during swap.
            if (points[1] > points[2]) {
                points[4] = points[1];
                points[1] = points[2];
                points[2] = points[4];
            }
            if (points[2] > points[3]) {
                points[4] = points[2];
                points[2] = points[3];
                points[3] = points[4];
            }
            if (points[0] > points[1]) {
                points[4] = points[0];
                points[0] = points[1];
                points[1] = points[4];
            }

            // average the internal pair, and then do the math from centerB
            collision->_contactPoint = centerB + (0.5f * (points[1] + points[2])) * axisB 
                + (capsuleA->getRadius() - distance) * BA;
            return true;
        }
    }
    return false;
}

bool sphereList(const SphereShape* sphereA, const ListShape* listB, CollisionList& collisions) {
    bool touching = false;
    for (int i = 0; i < listB->size() && !collisions.isFull(); ++i) {
        const Shape* subShape = listB->getSubShape(i);
        int subType = subShape->getType();
        if (subType == Shape::SPHERE_SHAPE) {
            touching = sphereSphere(sphereA, static_cast<const SphereShape*>(subShape), collisions) || touching;
        } else if (subType == Shape::CAPSULE_SHAPE) {
            touching = sphereCapsule(sphereA, static_cast<const CapsuleShape*>(subShape), collisions) || touching;
        }
    }
    return touching;
}

bool capsuleList(const CapsuleShape* capsuleA, const ListShape* listB, CollisionList& collisions) {
    bool touching = false;
    for (int i = 0; i < listB->size() && !collisions.isFull(); ++i) {
        const Shape* subShape = listB->getSubShape(i);
        int subType = subShape->getType();
        if (subType == Shape::SPHERE_SHAPE) {
            touching = capsuleSphere(capsuleA, static_cast<const SphereShape*>(subShape), collisions) || touching;
        } else if (subType == Shape::CAPSULE_SHAPE) {
            touching = capsuleCapsule(capsuleA, static_cast<const CapsuleShape*>(subShape), collisions) || touching;
        }
    }
    return touching;
}

bool listSphere(const ListShape* listA, const SphereShape* sphereB, CollisionList& collisions) {
    bool touching = false;
    for (int i = 0; i < listA->size() && !collisions.isFull(); ++i) {
        const Shape* subShape = listA->getSubShape(i);
        int subType = subShape->getType();
        if (subType == Shape::SPHERE_SHAPE) {
            touching = sphereSphere(static_cast<const SphereShape*>(subShape), sphereB, collisions) || touching;
        } else if (subType == Shape::CAPSULE_SHAPE) {
            touching = capsuleSphere(static_cast<const CapsuleShape*>(subShape), sphereB, collisions) || touching;
        }
    }
    return touching;
}

bool listCapsule(const ListShape* listA, const CapsuleShape* capsuleB, CollisionList& collisions) {
    bool touching = false;
    for (int i = 0; i < listA->size() && !collisions.isFull(); ++i) {
        const Shape* subShape = listA->getSubShape(i);
        int subType = subShape->getType();
        if (subType == Shape::SPHERE_SHAPE) {
            touching = sphereCapsule(static_cast<const SphereShape*>(subShape), capsuleB, collisions) || touching;
        } else if (subType == Shape::CAPSULE_SHAPE) {
            touching = capsuleCapsule(static_cast<const CapsuleShape*>(subShape), capsuleB, collisions) || touching;
        }
    }
    return touching;
}

bool listList(const ListShape* listA, const ListShape* listB, CollisionList& collisions) {
    bool touching = false;
    for (int i = 0; i < listA->size() && !collisions.isFull(); ++i) {
        const Shape* subShape = listA->getSubShape(i);
        for (int j = 0; j < listB->size() && !collisions.isFull(); ++j) {
            touching = shapeShape(subShape, listB->getSubShape(j), collisions) || touching;
        }
    }
    return touching;
}

}   // namespace ShapeCollider
