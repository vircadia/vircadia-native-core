//
//  SphereShape.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows on 2014.06.17
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/norm.hpp>

#include "SphereShape.h"

bool SphereShape::findRayIntersection(RayIntersectionInfo& intersection) const {
    float r2 = _boundingRadius * _boundingRadius;

    // compute closest approach (CA)
    float a = glm::dot(_translation - intersection._rayStart, intersection._rayDirection); // a = distance from ray-start to CA
    float b2 = glm::distance2(_translation, intersection._rayStart + a * intersection._rayDirection); // b2 = squared distance from sphere-center to CA
    if (b2 > r2) {
        // ray does not hit sphere
        return false;
    }
    float c = sqrtf(r2 - b2); // c = distance from CA to sphere surface along intersection._rayDirection
    float d2 = glm::distance2(intersection._rayStart, _translation); // d2 = squared distance from sphere-center to ray-start
    float distance = FLT_MAX;
    if (a < 0.0f) {
        // ray points away from sphere-center
        if (d2 > r2) {
            // ray starts outside sphere
            return false;
        }
        // ray starts inside sphere
        distance = c + a;
    } else if (d2 > r2) {
        // ray starts outside sphere
        distance = a - c;
    } else {
        // ray starts inside sphere
        distance = a + c;
    }
    if (distance > 0.0f && distance < intersection._rayLength && distance < intersection._hitDistance) {
        intersection._hitDistance = distance;
        intersection._hitNormal = glm::normalize(intersection._rayStart + distance * intersection._rayDirection - _translation);
        return true;
    }
    return false;
}
