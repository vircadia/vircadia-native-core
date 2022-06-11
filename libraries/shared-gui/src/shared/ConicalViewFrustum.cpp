//
//  ConicalViewFrustum.cpp
//  libraries/shared-gui/src/shared
//
//  Created by Clement Brisset 4/26/18
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ConicalViewFrustum.h"


#include <NumericalConstants.h>
#include "../ViewFrustum.h"


void ConicalViewFrustum::set(const ViewFrustum& viewFrustum) {
    _data.set(ViewFrustum.getPosition(), viewFrustum.getCenterRadius(), viewFrustum.getFarClip(), {
        viewFrustum.getNearTopLeft(),
        viewFrustum.getNearTopRight(),
        viewFrustum.getNearBottomLeft(),
        viewFrustum.getNearBottomRight(),
    });
}

void ConicalViewFrustum::calculate() {
    // Pre-compute cos and sin for faster checks
    _cosAngle = cosf(_data.angle);
    _sinAngle = sqrtf(1.0f - _cosAngle * _cosAngle);
}

bool ConicalViewFrustum::isVerySimilar(const ConicalViewFrustum& other) const {
    return _data.isVerySimilar(other._data);
}

bool ConicalViewFrustum::intersects(const AACube& cube) const {
    auto radius = 0.5f * SQRT_THREE * cube.getScale(); // radius of bounding sphere
    auto position = cube.calcCenter() - _data.position; // position of bounding sphere in view-frame
    float distance = glm::length(position);

    return intersects(position, distance, radius);
}

bool ConicalViewFrustum::intersects(const AABox& box) const {
    auto radius = 0.5f * glm::length(box.getScale()); // radius of bounding sphere
    auto position = box.calcCenter() - _data.position; // position of bounding sphere in view-frame
    float distance = glm::length(position);

    return intersects(position, distance, radius);
}

float ConicalViewFrustum::getAngularSize(const AACube& cube) const {
    auto radius = 0.5f * SQRT_THREE * cube.getScale(); // radius of bounding sphere
    auto position = cube.calcCenter() - _data.position; // position of bounding sphere in view-frame
    float distance = glm::length(position);

    return getAngularSize(distance, radius);
}

float ConicalViewFrustum::getAngularSize(const AABox& box) const {
    auto radius = 0.5f * glm::length(box.getScale()); // radius of bounding sphere
    auto position = box.calcCenter() - _data.position; // position of bounding sphere in view-frame
    float distance = glm::length(position);

    return getAngularSize(distance, radius);
}


bool ConicalViewFrustum::intersects(const glm::vec3& relativePosition, float distance, float radius) const {
    if (distance < _data.radius + radius) {
        // Inside keyhole radius
        return true;
    }
    if (distance > _data.farClip + radius) {
        // Past far clip
        return false;
    }

    // We check the angle between the center of the cube and the direction of the view.
    // If it is less than the sum of the half-angle from center of cone to outer edge plus
    // the half apparent angle of the bounding sphere then it is in view.
    //
    // The math here is left as an exercise for the reader with the following hints:
    // (1) We actually check the dot product of the cube's local position rather than the angle and
    // (2) we take advantage of this trig identity: cos(A+B) = cos(A)*cos(B) - sin(A)*sin(B)
    return glm::dot(relativePosition, _data.direction) >
           sqrtf(distance * distance - radius * radius) * _cosAngle - radius * _sinAngle;
}

float ConicalViewFrustum::getAngularSize(float distance, float radius) const {
    const float AVOID_DIVIDE_BY_ZERO = 0.001f;
    float angularSize = radius / (distance + AVOID_DIVIDE_BY_ZERO);
    return angularSize;
}

int ConicalViewFrustum::serialize(unsigned char* destinationBuffer) const {
    return _data.serialize(destinationBuffer);
}

int ConicalViewFrustum::deserialize(const unsigned char* sourceBuffer) {
    auto size = _data.deserialize(sourceBuffer);
    calculate();
    return size;
}

void ConicalViewFrustum::setPositionAndSimpleRadius(const glm::vec3& position, float radius) {
    _data.position = position;
    _data.radius = radius;
    _data.farClip = radius / 2.0f;
}
