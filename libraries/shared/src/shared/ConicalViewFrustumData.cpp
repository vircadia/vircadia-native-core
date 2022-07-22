//
//  ConicalViewFrustumData.cpp
//  libraries/shared/src/shared
//
//  Created by Nshan G. on 31 May 2022.
//  Copyright 2014 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ConicalViewFrustumData.h"

#include <glm/gtc/type_ptr.hpp>

int ConicalViewFrustumData::serialize(unsigned char* destinationBuffer) const {
    const unsigned char* startPosition = destinationBuffer;

    memcpy(destinationBuffer, &position, sizeof(position));
    destinationBuffer += sizeof(position);
    memcpy(destinationBuffer, &direction, sizeof(direction));
    destinationBuffer += sizeof(direction);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, angle);
    destinationBuffer += packClipValueToTwoByte(destinationBuffer, farClip);
    memcpy(destinationBuffer, &radius, sizeof(radius));
    destinationBuffer += sizeof(radius);

    return destinationBuffer - startPosition;
}

int ConicalViewFrustumData::deserialize(const unsigned char* sourceBuffer) {
    const unsigned char* startPosition = sourceBuffer;

    memcpy(glm::value_ptr(position), sourceBuffer, sizeof(position));
    sourceBuffer += sizeof(position);
    memcpy(glm::value_ptr(direction), sourceBuffer, sizeof(direction));
    sourceBuffer += sizeof(direction);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*)sourceBuffer, &angle);
    sourceBuffer += unpackClipValueFromTwoByte(sourceBuffer, farClip);
    memcpy(&radius, sourceBuffer, sizeof(radius));
    sourceBuffer += sizeof(radius);

    return sourceBuffer - startPosition;
}

bool ConicalViewFrustumData::isVerySimilar(const ConicalViewFrustumData& other) const {
    const float MIN_POSITION_SLOP_SQUARED = 25.0f; // 5 meters squared
    const float MIN_ANGLE_BETWEEN = 0.174533f; // radian angle between 2 vectors 10 degrees apart
    const float MIN_RELATIVE_ERROR = 0.01f; // 1%

    return glm::distance2(position, other.position) < MIN_POSITION_SLOP_SQUARED &&
            angleBetween(direction, other.direction) < MIN_ANGLE_BETWEEN &&
            closeEnough(angle, other.angle, MIN_RELATIVE_ERROR) &&
            closeEnough(farClip, other.farClip, MIN_RELATIVE_ERROR) &&
            closeEnough(radius, other.radius, MIN_RELATIVE_ERROR);
}

void ConicalViewFrustumData::set(glm::vec3 position, float radius, float farClip, std::array<glm::vec3, 4> nearCorners) {
    this->position = position;
    this->radius = radius;
    this->farClip = farClip;

    auto topLeft = nearCorners[0] - position;
    auto topRight = nearCorners[1] - position;
    auto bottomLeft = nearCorners[2] - position;
    auto bottomRight = nearCorners[3] - position;
    auto centerAxis = 0.25f * (topLeft + topRight + bottomLeft + bottomRight); // Take the average

    direction = glm::normalize(centerAxis);
    angle = std::max(std::max(angleBetween(direction, topLeft),
                               angleBetween(direction, topRight)),
                      std::max(angleBetween(direction, bottomLeft),
                               angleBetween(direction, bottomRight)));
}

