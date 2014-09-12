//
//  Extents.cpp
//  libraries/shared/src
//
//  Created by Andrzej Kapolka on 9/18/13.
//  Moved to shared by Brad Hefta-Gaub on 9/11/14
//  Copyright 2013-2104 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include "Extents.h"

void Extents::reset() {
    minimum = glm::vec3(FLT_MAX);
    maximum = glm::vec3(-FLT_MAX);
}

bool Extents::containsPoint(const glm::vec3& point) const {
    return (point.x >= minimum.x && point.x <= maximum.x
        && point.y >= minimum.y && point.y <= maximum.y
        && point.z >= minimum.z && point.z <= maximum.z);
}

void Extents::addExtents(const Extents& extents) {
     minimum = glm::min(minimum, extents.minimum);
     maximum = glm::max(maximum, extents.maximum);
}

void Extents::addPoint(const glm::vec3& point) {
    minimum = glm::min(minimum, point);
    maximum = glm::max(maximum, point);
}

void Extents::rotate(const glm::quat& rotation) {
    glm::vec3 bottomLeftNear(minimum.x, minimum.y, minimum.z);
    glm::vec3 bottomRightNear(maximum.x, minimum.y, minimum.z);
    glm::vec3 bottomLeftFar(minimum.x, minimum.y, maximum.z);
    glm::vec3 bottomRightFar(maximum.x, minimum.y, maximum.z);
    glm::vec3 topLeftNear(minimum.x, maximum.y, minimum.z);
    glm::vec3 topRightNear(maximum.x, maximum.y, minimum.z);
    glm::vec3 topLeftFar(minimum.x, maximum.y, maximum.z);
    glm::vec3 topRightFar(maximum.x, maximum.y, maximum.z);

    glm::vec3 bottomLeftNearRotated =  rotation * bottomLeftNear;
    glm::vec3 bottomRightNearRotated = rotation * bottomRightNear;
    glm::vec3 bottomLeftFarRotated = rotation * bottomLeftFar;
    glm::vec3 bottomRightFarRotated = rotation * bottomRightFar;
    glm::vec3 topLeftNearRotated = rotation * topLeftNear;
    glm::vec3 topRightNearRotated = rotation * topRightNear;
    glm::vec3 topLeftFarRotated = rotation * topLeftFar;
    glm::vec3 topRightFarRotated = rotation * topRightFar;
    
    minimum = glm::min(bottomLeftNearRotated,
                        glm::min(bottomRightNearRotated,
                        glm::min(bottomLeftFarRotated,
                        glm::min(bottomRightFarRotated,
                        glm::min(topLeftNearRotated,
                        glm::min(topRightNearRotated,
                        glm::min(topLeftFarRotated,topRightFarRotated)))))));

    maximum = glm::max(bottomLeftNearRotated,
                        glm::max(bottomRightNearRotated,
                        glm::max(bottomLeftFarRotated,
                        glm::max(bottomRightFarRotated,
                        glm::max(topLeftNearRotated,
                        glm::max(topRightNearRotated,
                        glm::max(topLeftFarRotated,topRightFarRotated)))))));
}
