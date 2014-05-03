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

#include "GeometryUtil.h"
#include "ShapeCollider.h"

// NOTE:
//
// * Large ListShape's are inefficient keep the lists short.
// * Collisions between lists of lists work in theory but are not recommended.

namespace ShapeCollider {

bool collideShapes(const Shape* shapeA, const Shape* shapeB, CollisionList& collisions) {
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
        } else if (typeB == Shape::PLANE_SHAPE) {
            return spherePlane(sphereA, static_cast<const PlaneShape*>(shapeB), collisions);
        }
    } else if (typeA == Shape::CAPSULE_SHAPE) {
        const CapsuleShape* capsuleA = static_cast<const CapsuleShape*>(shapeA);
        if (typeB == Shape::SPHERE_SHAPE) {
            return capsuleSphere(capsuleA, static_cast<const SphereShape*>(shapeB), collisions);
        } else if (typeB == Shape::CAPSULE_SHAPE) {
            return capsuleCapsule(capsuleA, static_cast<const CapsuleShape*>(shapeB), collisions);
        } else if (typeB == Shape::PLANE_SHAPE) {
            return capsulePlane(capsuleA, static_cast<const PlaneShape*>(shapeB), collisions);
        }
    } else if (typeA == Shape::PLANE_SHAPE) {
        const PlaneShape* planeA = static_cast<const PlaneShape*>(shapeA);
        if (typeB == Shape::SPHERE_SHAPE) {
            return planeSphere(planeA, static_cast<const SphereShape*>(shapeB), collisions);
        } else if (typeB == Shape::CAPSULE_SHAPE) {
            return planeCapsule(planeA, static_cast<const CapsuleShape*>(shapeB), collisions);
        } else if (typeB == Shape::PLANE_SHAPE) {
            return planePlane(planeA, static_cast<const PlaneShape*>(shapeB), collisions);
        }
    } else if (typeA == Shape::LIST_SHAPE) {
        const ListShape* listA = static_cast<const ListShape*>(shapeA);
        if (typeB == Shape::SPHERE_SHAPE) {
            return listSphere(listA, static_cast<const SphereShape*>(shapeB), collisions);
        } else if (typeB == Shape::CAPSULE_SHAPE) {
            return listCapsule(listA, static_cast<const CapsuleShape*>(shapeB), collisions);
        } else if (typeB == Shape::PLANE_SHAPE) {
            return listPlane(listA, static_cast<const PlaneShape*>(shapeB), collisions);
        }
    }
    return false;
}

static CollisionList tempCollisions(32);

bool collideShapesCoarse(const QVector<const Shape*>& shapesA, const QVector<const Shape*>& shapesB, CollisionInfo& collision) {
    tempCollisions.clear();
    foreach (const Shape* shapeA, shapesA) {
        foreach (const Shape* shapeB, shapesB) {
            ShapeCollider::collideShapes(shapeA, shapeB, tempCollisions);
        }
    }
    if (tempCollisions.size() > 0) {
        glm::vec3 totalPenetration(0.0f);
        glm::vec3 averageContactPoint(0.0f);
        for (int j = 0; j < tempCollisions.size(); ++j) {
            CollisionInfo* c = tempCollisions.getCollision(j);
            totalPenetration = addPenetrations(totalPenetration, c->_penetration);
            averageContactPoint += c->_contactPoint;
        }
        collision._penetration = totalPenetration;
        collision._contactPoint = averageContactPoint / (float)(tempCollisions.size());
        return true;
    }
    return false;
}

bool collideShapeWithAACube(const Shape* shapeA, const glm::vec3& cubeCenter, float cubeSide, CollisionList& collisions) {
    int typeA = shapeA->getType();
    if (typeA == Shape::SPHERE_SHAPE) {
        return sphereAACube(static_cast<const SphereShape*>(shapeA), cubeCenter, cubeSide, collisions);
    } else if (typeA == Shape::CAPSULE_SHAPE) {
        return capsuleAACube(static_cast<const CapsuleShape*>(shapeA), cubeCenter, cubeSide, collisions);
    } else if (typeA == Shape::LIST_SHAPE) {
        const ListShape* listA = static_cast<const ListShape*>(shapeA);
        bool touching = false;
        for (int i = 0; i < listA->size() && !collisions.isFull(); ++i) {
            const Shape* subShape = listA->getSubShape(i);
            int subType = subShape->getType();
            if (subType == Shape::SPHERE_SHAPE) {
                touching = sphereAACube(static_cast<const SphereShape*>(subShape), cubeCenter, cubeSide, collisions) || touching;
            } else if (subType == Shape::CAPSULE_SHAPE) {
                touching = capsuleAACube(static_cast<const CapsuleShape*>(subShape), cubeCenter, cubeSide, collisions) || touching;
            }
        }
        return touching;
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
            BA = glm::vec3(0.0f, 1.0f, 0.0f);
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
            float sign = (axialDistance > 0.0f) ? 1.0f : -1.0f;
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
            if (axialDistance < 0.0f) {
                // we're hitting the start cap, so we negate the capsuleAxis
                capsuleAxis *= -1;
            }
            // penetration points from A into B
            float sign = (axialDistance > 0.0f) ? -1.0f : 1.0f;
            collision->_penetration = (sign * (totalRadius + capsuleB->getHalfHeight() - absAxialDistance)) * capsuleAxis;
            // contactPoint is on surface of sphereA
            collision->_contactPoint = sphereA->getPosition() + (sign * sphereA->getRadius()) * capsuleAxis;
        }
        return true;
    }
    return false;
}

bool spherePlane(const SphereShape* sphereA, const PlaneShape* planeB, CollisionList& collisions) {
    glm::vec3 penetration;
    if (findSpherePlanePenetration(sphereA->getPosition(), sphereA->getRadius(), planeB->getCoefficients(), penetration)) {
        CollisionInfo* collision = collisions.getNewCollision();
        if (!collision) {
            return false; // collision list is full
        }
        collision->_penetration = penetration;
        collision->_contactPoint = sphereA->getPosition() + sphereA->getRadius() * glm::normalize(penetration);
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
            float sign = (axialDistance > 0.0f) ? 1.0f : -1.0f;
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
                if (axialDistance < 0.0f) {
                    // we're hitting the start cap, so we negate the capsuleAxis
                    capsuleAxis *= -1;
                }
                float sign = (axialDistance > 0.0f) ? 1.0f : -1.0f;
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
    float denominator = 1.0f - aDotB * aDotB;
    float totalRadius = capsuleA->getRadius() + capsuleB->getRadius();
    if (denominator > EPSILON) {
        // distances to points of closest approach
        float distanceA = glm::dot((centerB - centerA), (axisA - (aDotB) * axisB)) / denominator;
        float distanceB = glm::dot((centerA - centerB), (axisB - (aDotB) * axisA)) / denominator;
        
        // clamp the distances to the ends of the capsule line segments
        float absDistanceA = fabs(distanceA);
        if (absDistanceA > capsuleA->getHalfHeight() + capsuleA->getRadius()) {
            float signA = distanceA < 0.0f ? -1.0f : 1.0f;
            distanceA = signA * capsuleA->getHalfHeight();
        }
        float absDistanceB = fabs(distanceB);
        if (absDistanceB > capsuleB->getHalfHeight() + capsuleB->getRadius()) {
            float signB = distanceB < 0.0f ? -1.0f : 1.0f;
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
                    BA = glm::vec3(0.0f, 1.0f, 0.0f);
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
                BA = glm::vec3(0.0f, 1.0f, 0.0f);
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

bool capsulePlane(const CapsuleShape* capsuleA, const PlaneShape* planeB, CollisionList& collisions) {
    glm::vec3 start, end, penetration;
    capsuleA->getStartPoint(start);
    capsuleA->getEndPoint(end);
    glm::vec4 plane = planeB->getCoefficients();
    if (findCapsulePlanePenetration(start, end, capsuleA->getRadius(), plane, penetration)) {
        CollisionInfo* collision = collisions.getNewCollision();
        if (!collision) {
            return false; // collision list is full
        }
        collision->_penetration = penetration;
        glm::vec3 deepestEnd = (glm::dot(start, glm::vec3(plane)) < glm::dot(end, glm::vec3(plane))) ? start : end;
        collision->_contactPoint = deepestEnd + capsuleA->getRadius() * glm::normalize(penetration);
        return true;
    }
    return false;
}

bool planeSphere(const PlaneShape* planeA, const SphereShape* sphereB, CollisionList& collisions) {
    glm::vec3 penetration;
    if (findSpherePlanePenetration(sphereB->getPosition(), sphereB->getRadius(), planeA->getCoefficients(), penetration)) {
        CollisionInfo* collision = collisions.getNewCollision();
        if (!collision) {
            return false; // collision list is full
        }
        collision->_penetration = -penetration;
        collision->_contactPoint = sphereB->getPosition() +
            (sphereB->getRadius() / glm::length(penetration) - 1.0f) * penetration;
        return true;
    }
    return false;
}

bool planeCapsule(const PlaneShape* planeA, const CapsuleShape* capsuleB, CollisionList& collisions) {
    glm::vec3 start, end, penetration;
    capsuleB->getStartPoint(start);
    capsuleB->getEndPoint(end);
    glm::vec4 plane = planeA->getCoefficients();
    if (findCapsulePlanePenetration(start, end, capsuleB->getRadius(), plane, penetration)) {
        CollisionInfo* collision = collisions.getNewCollision();
        if (!collision) {
            return false; // collision list is full
        }
        collision->_penetration = -penetration;
        glm::vec3 deepestEnd = (glm::dot(start, glm::vec3(plane)) < glm::dot(end, glm::vec3(plane))) ? start : end;
        collision->_contactPoint = deepestEnd + (capsuleB->getRadius() / glm::length(penetration) - 1.0f) * penetration;
        return true;
    }
    return false;
}

bool planePlane(const PlaneShape* planeA, const PlaneShape* planeB, CollisionList& collisions) {
    // technically, planes always collide unless they're parallel and not coincident; however, that's
    // not going to give us any useful information
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
        } else if (subType == Shape::PLANE_SHAPE) {
            touching = spherePlane(sphereA, static_cast<const PlaneShape*>(subShape), collisions) || touching;
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
        } else if (subType == Shape::PLANE_SHAPE) {
            touching = capsulePlane(capsuleA, static_cast<const PlaneShape*>(subShape), collisions) || touching;
        }
    }
    return touching;
}

bool planeList(const PlaneShape* planeA, const ListShape* listB, CollisionList& collisions) {
    bool touching = false;
    for (int i = 0; i < listB->size() && !collisions.isFull(); ++i) {
        const Shape* subShape = listB->getSubShape(i);
        int subType = subShape->getType();
        if (subType == Shape::SPHERE_SHAPE) {
            touching = planeSphere(planeA, static_cast<const SphereShape*>(subShape), collisions) || touching;
        } else if (subType == Shape::CAPSULE_SHAPE) {
            touching = planeCapsule(planeA, static_cast<const CapsuleShape*>(subShape), collisions) || touching;
        } else if (subType == Shape::PLANE_SHAPE) {
            touching = planePlane(planeA, static_cast<const PlaneShape*>(subShape), collisions) || touching;
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
        } else if (subType == Shape::PLANE_SHAPE) {
            touching = planeSphere(static_cast<const PlaneShape*>(subShape), sphereB, collisions) || touching;
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
        } else if (subType == Shape::PLANE_SHAPE) {
            touching = planeCapsule(static_cast<const PlaneShape*>(subShape), capsuleB, collisions) || touching;
        }
    }
    return touching;
}

bool listPlane(const ListShape* listA, const PlaneShape* planeB, CollisionList& collisions) {
    bool touching = false;
    for (int i = 0; i < listA->size() && !collisions.isFull(); ++i) {
        const Shape* subShape = listA->getSubShape(i);
        int subType = subShape->getType();
        if (subType == Shape::SPHERE_SHAPE) {
            touching = spherePlane(static_cast<const SphereShape*>(subShape), planeB, collisions) || touching;
        } else if (subType == Shape::CAPSULE_SHAPE) {
            touching = capsulePlane(static_cast<const CapsuleShape*>(subShape), planeB, collisions) || touching;
        } else if (subType == Shape::PLANE_SHAPE) {
            touching = planePlane(static_cast<const PlaneShape*>(subShape), planeB, collisions) || touching;
        }
    }
    return touching;
}

bool listList(const ListShape* listA, const ListShape* listB, CollisionList& collisions) {
    bool touching = false;
    for (int i = 0; i < listA->size() && !collisions.isFull(); ++i) {
        const Shape* subShape = listA->getSubShape(i);
        for (int j = 0; j < listB->size() && !collisions.isFull(); ++j) {
            touching = collideShapes(subShape, listB->getSubShape(j), collisions) || touching;
        }
    }
    return touching;
}

// helper function
bool sphereAACube(const glm::vec3& sphereCenter, float sphereRadius, const glm::vec3& cubeCenter, 
        float cubeSide, CollisionList& collisions) {
    // sphere is A
    // cube is B
    // BA = B - A = from center of A to center of B 
    float halfCubeSide = 0.5f * cubeSide;
    glm::vec3 BA = cubeCenter - sphereCenter;
    float distance = glm::length(BA);
    if (distance > EPSILON) {
        float maxBA = glm::max(glm::max(glm::abs(BA.x), glm::abs(BA.y)), glm::abs(BA.z));
        if (maxBA > halfCubeSide + sphereRadius) {
            // sphere misses cube entirely
            return false;
        } 
        CollisionInfo* collision = collisions.getNewCollision();
        if (!collision) {
            return false;
        }
        if (maxBA > halfCubeSide) {
            // sphere hits cube but its center is outside cube
            
            // contactPoint is on surface of sphere
            glm::vec3 cubeContact = glm::abs(BA);
            glm::vec3 direction = cubeContact - glm::vec3(halfCubeSide);

            if (direction.x < 0.0f) {
                direction.x = 0.0f;
            }
            if (direction.y < 0.0f) {
                direction.y = 0.0f;
            }
            if (direction.z < 0.0f) {
                direction.z = 0.0f;
            }

            glm::vec3 signs = glm::sign(BA);
            direction.x *= signs.x;
            direction.y *= signs.y;
            direction.z *= signs.z;
            direction = glm::normalize(direction);
            collision->_contactPoint = sphereCenter + sphereRadius * direction;

            // penetration points from contact point on cube to that on sphere
            if (cubeContact.x > halfCubeSide) {
                cubeContact.x = halfCubeSide;
            }
            cubeContact.x *= -signs.x;
            if (cubeContact.y > halfCubeSide) {
                cubeContact.y = halfCubeSide;
            }
            cubeContact.y *= -signs.y;
            if (cubeContact.z > halfCubeSide) {
                cubeContact.z = halfCubeSide;
            }
            cubeContact.z *= -signs.z;
            //collision->_penetration = collision->_contactPoint - cubeCenter + cubeContact;
            collision->_penetration = collision->_contactPoint - (cubeCenter + cubeContact);
        } else {
            // sphere center is inside cube
            // --> push out nearest face
            glm::vec3 direction;
            BA /= maxBA;
            glm::modf(BA, direction);
            direction = glm::normalize(direction); 

            // penetration is the projection of surfaceAB on direction
            collision->_penetration = (halfCubeSide + sphereRadius - distance * glm::dot(BA, direction)) * direction;
            // contactPoint is on surface of A
            collision->_contactPoint = sphereCenter + sphereRadius * direction;
        }
        return true;
    } else if (sphereRadius + halfCubeSide > distance) {
        // NOTE: for cocentric approximation we collide sphere and cube as two spheres which means 
        // this algorithm will probably be wrong when both sphere and cube are very small (both ~EPSILON)
        CollisionInfo* collision = collisions.getNewCollision();
        if (collision) {
            // the penetration and contactPoint are undefined, so we pick a penetration direction (-yAxis)
            collision->_penetration = (sphereRadius + halfCubeSide) * glm::vec3(0.0f, -1.0f, 0.0f);
            // contactPoint is on surface of A
            collision->_contactPoint = sphereCenter + collision->_penetration;
            return true;
        }
    }
    return false;
}

// helper function
/* KEEP THIS CODE -- this is how to collide the cube with stark face normals (no rounding).
* We might want to use this code later for sealing boundaries between adjacent voxels.
bool sphereAACube_StarkAngles(const glm::vec3& sphereCenter, float sphereRadius, const glm::vec3& cubeCenter, 
        float cubeSide, CollisionList& collisions) {
    glm::vec3 BA = cubeCenter - sphereCenter;
    float distance = glm::length(BA);
    if (distance > EPSILON) {
        BA /= distance; // BA is now normalized
        // compute the nearest point on sphere
        glm::vec3 surfaceA = sphereCenter + sphereRadius * BA;
        // compute the nearest point on cube
        float maxBA = glm::max(glm::max(fabs(BA.x), fabs(BA.y)), fabs(BA.z));
        glm::vec3 surfaceB = cubeCenter - (0.5f * cubeSide / maxBA) * BA;
        // collision happens when "vector to surfaceA from surfaceB" dots with BA to produce a positive value
        glm::vec3 surfaceAB = surfaceA - surfaceB;
        if (glm::dot(surfaceAB, BA) > 0.f) {
            CollisionInfo* collision = collisions.getNewCollision();
            if (collision) {
                // penetration is parallel to box side direction
                BA /= maxBA;
                glm::vec3 direction;
                glm::modf(BA, direction);
                direction = glm::normalize(direction); 

                // penetration is the projection of surfaceAB on direction
                collision->_penetration = glm::dot(surfaceAB, direction) * direction;
                // contactPoint is on surface of A
                collision->_contactPoint = sphereCenter + sphereRadius * direction;
                return true;
            }
        }
    } else if (sphereRadius + 0.5f * cubeSide > distance) {
        // NOTE: for cocentric approximation we collide sphere and cube as two spheres which means 
        // this algorithm will probably be wrong when both sphere and cube are very small (both ~EPSILON)
        CollisionInfo* collision = collisions.getNewCollision();
        if (collision) {
            // the penetration and contactPoint are undefined, so we pick a penetration direction (-yAxis)
            collision->_penetration = (sphereRadius + 0.5f * cubeSide) * glm::vec3(0.0f, -1.0f, 0.0f);
            // contactPoint is on surface of A
            collision->_contactPoint = sphereCenter + collision->_penetration;
            return true;
        }
    }
    return false;
}
*/

bool sphereAACube(const SphereShape* sphereA, const glm::vec3& cubeCenter, float cubeSide, CollisionList& collisions) {
    return sphereAACube(sphereA->getPosition(), sphereA->getRadius(), cubeCenter, cubeSide, collisions);
}

bool capsuleAACube(const CapsuleShape* capsuleA, const glm::vec3& cubeCenter, float cubeSide, CollisionList& collisions) {
    // find nerest approach of capsule line segment to cube
    glm::vec3 capsuleAxis;
    capsuleA->computeNormalizedAxis(capsuleAxis);
    float offset = glm::dot(cubeCenter - capsuleA->getPosition(), capsuleAxis);
    float halfHeight = capsuleA->getHalfHeight();
    if (offset > halfHeight) {
        offset = halfHeight;
    } else if (offset < -halfHeight) {
        offset = -halfHeight;
    }
    glm::vec3 nearestApproach = capsuleA->getPosition() + offset * capsuleAxis;
    // collide nearest approach like a sphere at that point
    return sphereAACube(nearestApproach, capsuleA->getRadius(), cubeCenter, cubeSide, collisions);
}


}   // namespace ShapeCollider
