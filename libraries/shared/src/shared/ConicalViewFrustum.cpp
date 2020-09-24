//
//  ConicalViewFrustum.cpp
//  libraries/shared/src/shared
//
//  Created by Clement Brisset 4/26/18
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ConicalViewFrustum.h"


#include "../NumericalConstants.h"
#include "../ViewFrustum.h"
#include <glm/gtc/type_ptr.hpp>

void ConicalViewFrustum::set(const ViewFrustum& viewFrustum) {
    // The ConicalViewFrustum has two parts: a central sphere (same as ViewFrustum) and a circular cone that bounds the frustum part.
    // Why?  Because approximate intersection tests are much faster to compute for a cone than for a frustum.
    _position = viewFrustum.getPosition();
    _radius = viewFrustum.getCenterRadius();
    _farClip = viewFrustum.getFarClip();

    auto topLeft = viewFrustum.getNearTopLeft() - _position;
    auto topRight = viewFrustum.getNearTopRight() - _position;
    auto bottomLeft = viewFrustum.getNearBottomLeft() - _position;
    auto bottomRight = viewFrustum.getNearBottomRight() - _position;
    auto centerAxis = 0.25f * (topLeft + topRight + bottomLeft + bottomRight); // Take the average

    _direction = glm::normalize(centerAxis);
    _angle = std::max(std::max(angleBetween(_direction, topLeft),
                               angleBetween(_direction, topRight)),
                      std::max(angleBetween(_direction, bottomLeft),
                               angleBetween(_direction, bottomRight)));
}

void ConicalViewFrustum::calculate() {
    // Pre-compute cos and sin for faster checks
    _cosAngle = cosf(_angle);
    _sinAngle = sqrtf(1.0f - _cosAngle * _cosAngle);
}

bool ConicalViewFrustum::isVerySimilar(const ConicalViewFrustum& other) const {
    const float MIN_POSITION_SLOP_SQUARED = 25.0f; // 5 meters squared
    const float MIN_ANGLE_BETWEEN = 0.174533f; // radian angle between 2 vectors 10 degrees apart
    const float MIN_RELATIVE_ERROR = 0.01f; // 1%

    return glm::distance2(_position, other._position) < MIN_POSITION_SLOP_SQUARED &&
            angleBetween(_direction, other._direction) < MIN_ANGLE_BETWEEN &&
            closeEnough(_angle, other._angle, MIN_RELATIVE_ERROR) &&
            closeEnough(_farClip, other._farClip, MIN_RELATIVE_ERROR) &&
            closeEnough(_radius, other._radius, MIN_RELATIVE_ERROR);
}

bool ConicalViewFrustum::intersects(const AACube& cube) const {
    auto radius = 0.5f * SQRT_THREE * cube.getScale(); // radius of bounding sphere
    auto position = cube.calcCenter() - _position; // position of bounding sphere in view-frame
    float distance = glm::length(position);

    return intersects(position, distance, radius);
}

bool ConicalViewFrustum::intersects(const AABox& box) const {
    auto radius = 0.5f * glm::length(box.getScale()); // radius of bounding sphere
    auto position = box.calcCenter() - _position; // position of bounding sphere in view-frame
    float distance = glm::length(position);

    return intersects(position, distance, radius);
}

float ConicalViewFrustum::getAngularSize(const AACube& cube) const {
    auto radius = 0.5f * SQRT_THREE * cube.getScale(); // radius of bounding sphere
    auto position = cube.calcCenter() - _position; // position of bounding sphere in view-frame
    float distance = glm::length(position);

    return getAngularSize(distance, radius);
}

float ConicalViewFrustum::getAngularSize(const AABox& box) const {
    auto radius = 0.5f * glm::length(box.getScale()); // radius of bounding sphere
    auto position = box.calcCenter() - _position; // position of bounding sphere in view-frame
    float distance = glm::length(position);

    return getAngularSize(distance, radius);
}


bool ConicalViewFrustum::intersects(const glm::vec3& relativePosition, float distance, float radius) const {
    if (distance < _radius + radius) {
        // Inside keyhole radius
        return true;
    }
    if (distance > _farClip + radius) {
        // Past far clip
        return false;
    }

    // We check the angle between the center of the cube and the _direction of the view.
    // If it is less than the sum of the half-angle from center of cone to outer edge plus
    // the half apparent angle of the bounding sphere then it is in view.
    //
    // The math here is left as an exercise for the reader with the following hints:
    // (1) We actually check the dot product of the cube's local position rather than the angle and
    // (2) we take advantage of this trig identity: cos(A+B) = cos(A)*cos(B) - sin(A)*sin(B)
    return glm::dot(relativePosition, _direction) >
           sqrtf(distance * distance - radius * radius) * _cosAngle - radius * _sinAngle;
}

float ConicalViewFrustum::getAngularSize(float distance, float radius) const {
    const float AVOID_DIVIDE_BY_ZERO = 0.001f;
    float angularSize = radius / (distance + AVOID_DIVIDE_BY_ZERO);
    return angularSize;
}

int ConicalViewFrustum::serialize(unsigned char* destinationBuffer) const {
    const unsigned char* startPosition = destinationBuffer;

    memcpy(destinationBuffer, &_position, sizeof(_position));
    destinationBuffer += sizeof(_position);
    memcpy(destinationBuffer, &_direction, sizeof(_direction));
    destinationBuffer += sizeof(_direction);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _angle);
    destinationBuffer += packClipValueToTwoByte(destinationBuffer, _farClip);
    memcpy(destinationBuffer, &_radius, sizeof(_radius));
    destinationBuffer += sizeof(_radius);

    return destinationBuffer - startPosition;
}

int ConicalViewFrustum::deserialize(const unsigned char* sourceBuffer) {
    const unsigned char* startPosition = sourceBuffer;

    memcpy(glm::value_ptr(_position), sourceBuffer, sizeof(_position));
    sourceBuffer += sizeof(_position);
    memcpy(glm::value_ptr(_direction), sourceBuffer, sizeof(_direction));
    sourceBuffer += sizeof(_direction);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*)sourceBuffer, &_angle);
    sourceBuffer += unpackClipValueFromTwoByte(sourceBuffer, _farClip);
    memcpy(&_radius, sourceBuffer, sizeof(_radius));
    sourceBuffer += sizeof(_radius);

    calculate();

    return sourceBuffer - startPosition;
}

void ConicalViewFrustum::setPositionAndSimpleRadius(const glm::vec3& position, float radius) {
    _position = position;
    _radius = radius;
    _farClip = radius / 2.0f;
}
