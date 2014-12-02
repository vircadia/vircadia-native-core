//
//  AACubeShape.cpp
//  libraries/physics/src
//
//  Created by Andrew Meadows on 2014.08.22
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include "AACubeShape.h"
#include "SharedUtil.h" // for SQUARE_ROOT_OF_3 

glm::vec3 faceNormals[3] = { glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };

bool AACubeShape::findRayIntersection(RayIntersectionInfo& intersection) const {
    // A = ray point
    // B = cube center
    glm::vec3 BA = _translation - intersection._rayStart;

    // check for ray intersection with cube's bounding sphere
    // a = distance along line to closest approach to B
    float a = glm::dot(intersection._rayDirection, BA);
    // b2 = squared distance from cube center to point of closest approach
    float b2 = glm::length2(a * intersection._rayDirection - BA);
    // r = bounding radius of cube
    float halfSide = 0.5f * _scale;
    const float r = SQUARE_ROOT_OF_3 * halfSide;
    if (b2 > r * r) {
        // line doesn't hit cube's bounding sphere
        return false;
    }

    // check for tuncated/short ray
    // maxLength = maximum possible distance between rayStart and center of cube
    const float maxLength = glm::min(intersection._rayLength, intersection._hitDistance) + r;
    if (a * a + b2 > maxLength * maxLength) {
        // ray is not long enough to reach cube's bounding sphere
        // NOTE: we don't fall in here when ray's length if FLT_MAX because maxLength^2 will be inf or nan
        return false;
    }

    // the trivial checks have been exhausted, so must trace to each face
    bool hit = false;
    for (int i = 0; i < 3; ++i) {
        for (float sign = -1.0f; sign < 2.0f; sign += 2.0f) {
            glm::vec3 faceNormal = sign * faceNormals[i];
            float rayDotPlane = glm::dot(intersection._rayDirection, faceNormal);
            if (glm::abs(rayDotPlane) > EPSILON) {
                float distanceToFace = (halfSide + glm::dot(BA, faceNormal)) / rayDotPlane;
                if (distanceToFace >= 0.0f) {
                    glm::vec3 point = distanceToFace * intersection._rayDirection - BA;
                    int j = (i + 1) % 3;
                    int k = (i + 2) % 3;
                    glm::vec3 secondNormal = faceNormals[j];
                    glm::vec3 thirdNormal = faceNormals[k];
                    if (glm::abs(glm::dot(point, secondNormal)) > halfSide ||
                            glm::abs(glm::dot(point, thirdNormal)) > halfSide) {
                        continue;
                    }
                    if (distanceToFace < intersection._hitDistance && distanceToFace < intersection._rayLength) {
                        intersection._hitDistance = distanceToFace;
                        intersection._hitNormal = faceNormal;
                        intersection._hitShape = const_cast<AACubeShape*>(this);
                        hit = true;
                    }
                }
            }
        }
    }
    return hit;
}
