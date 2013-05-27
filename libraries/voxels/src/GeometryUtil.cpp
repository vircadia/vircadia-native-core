//
//  GeometryUtil.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <SharedUtil.h>

#include "GeometryUtil.h"

glm::vec3 computeVectorFromPointToSegment(const glm::vec3& point, const glm::vec3& start, const glm::vec3& end) {
    // compute the projection of the point vector onto the segment vector
    glm::vec3 segmentVector = end - start;
    float lengthSquared = glm::dot(segmentVector, segmentVector);
    if (lengthSquared < EPSILON) {
        return start - point; // start and end the same
    }
    float proj = glm::dot(point - start, segmentVector) / lengthSquared;
    if (proj <= 0.0f) { // closest to the start
        return start - point;
        
    } else if (proj >= 1.0f) { // closest to the end
        return end - point;
    
    } else { // closest to the middle
        return start + segmentVector*proj - point;
    }
}

bool findSpherePenetration(const glm::vec3& penetratorToPenetratee, const glm::vec3& direction,
                           float combinedRadius, glm::vec3& penetration) {
    float vectorLength = glm::length(penetratorToPenetratee);
    if (vectorLength < EPSILON) {
        penetration = direction * combinedRadius;
        return true;
    }
    float distance = vectorLength - combinedRadius;
    if (distance < 0.0f) {
        penetration = penetratorToPenetratee * (-distance / vectorLength);
        return true;
    }
    return false;
}

bool findSpherePointPenetration(const glm::vec3& penetratorCenter, float penetratorRadius,
                                const glm::vec3& penetrateeLocation, glm::vec3& penetration) {
    return findSpherePenetration(penetrateeLocation - penetratorCenter, glm::vec3(0.0f, -1.0f, 0.0f),
        penetratorRadius, penetration);
}

bool findSphereSpherePenetration(const glm::vec3& penetratorCenter, float penetratorRadius,
                                 const glm::vec3& penetrateeCenter, float penetrateeRadius, glm::vec3& penetration) {
    return findSpherePointPenetration(penetratorCenter, penetratorRadius + penetrateeRadius, penetrateeCenter, penetration);
}

bool findSphereSegmentPenetration(const glm::vec3& penetratorCenter, float penetratorRadius,
                                  const glm::vec3& penetrateeStart, const glm::vec3& penetrateeEnd, glm::vec3& penetration) {
    return findSpherePenetration(computeVectorFromPointToSegment(penetratorCenter, penetrateeStart, penetrateeEnd),
        glm::vec3(0.0f, -1.0f, 0.0f), penetratorRadius, penetration);
}

bool findSphereCapsulePenetration(const glm::vec3& penetratorCenter, float penetratorRadius, const glm::vec3& penetrateeStart,
                                  const glm::vec3& penetrateeEnd, float penetrateeRadius, glm::vec3& penetration) {
    return findSphereSegmentPenetration(penetratorCenter, penetratorRadius + penetrateeRadius,
        penetrateeStart, penetrateeEnd, penetration);
}

bool findSpherePlanePenetration(const glm::vec3& penetratorCenter, float penetratorRadius, 
                                const glm::vec4& penetrateePlane, glm::vec3& penetration) {
    float distance = glm::dot(penetrateePlane, glm::vec4(penetratorCenter, 1.0f)) - penetratorRadius;
    if (distance < 0.0f) {
        penetration = glm::vec3(penetrateePlane) * distance;
        return true;
    }
    return false;
}

bool findCapsuleSpherePenetration(const glm::vec3& penetratorStart, const glm::vec3& penetratorEnd, float penetratorRadius,
                                  const glm::vec3& penetrateeCenter, float penetrateeRadius, glm::vec3& penetration) {
    if (findSphereCapsulePenetration(penetrateeCenter, penetrateeRadius,
            penetratorStart, penetratorEnd, penetratorRadius, penetration)) {
        penetration = -penetration;
        return true;
    }
    return false;
}

bool findCapsulePlanePenetration(const glm::vec3& penetratorStart, const glm::vec3& penetratorEnd, float penetratorRadius,
                                 const glm::vec4& penetrateePlane, glm::vec3& penetration) {
    float distance = glm::min(glm::dot(penetrateePlane, glm::vec4(penetratorStart, 1.0f)),
        glm::dot(penetrateePlane, glm::vec4(penetratorEnd, 1.0f))) - penetratorRadius;
    if (distance < 0.0f) {
        penetration = glm::vec3(penetrateePlane) * distance;
        return true;
    }
    return false;
}

glm::vec3 addPenetrations(const glm::vec3& currentPenetration, const glm::vec3& newPenetration) {
    // find the component of the new penetration in the direction of the current
    float currentLength = glm::length(currentPenetration);
    if (currentLength == 0.0f) {
        return newPenetration;
    }
    glm::vec3 currentDirection = currentPenetration / currentLength;
    float directionalComponent = glm::dot(newPenetration, currentDirection);
    
    // if orthogonal or in the opposite direction, we can simply add
    if (directionalComponent <= 0.0f) {
        return currentPenetration + newPenetration;
    }
    
    // otherwise, we need to take the maximum component of current and new
    return currentDirection * glm::max(directionalComponent, currentLength) +
        newPenetration - (currentDirection * directionalComponent);
}
