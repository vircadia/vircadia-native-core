//
//  ElbowConstraint.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ElbowConstraint.h"

#include <algorithm>

#include <GeometryUtil.h>
#include <NumericalConstants.h>
#include "AnimUtil.h"

ElbowConstraint::ElbowConstraint() :
        _minAngle(-PI),
        _maxAngle(PI)
{
}

void ElbowConstraint::setHingeAxis(const glm::vec3& axis) {
    float axisLength = glm::length(axis);
    assert(axisLength > EPSILON);
    _axis = axis / axisLength;

    // for later we need a second axis that is perpendicular to the first
    for (int i = 0; i < 3; ++i) {
        float component = _axis[i];
        const float MIN_LARGEST_NORMALIZED_VECTOR_COMPONENT = 0.57735f; // just under 1/sqrt(3)
        if (fabsf(component) > MIN_LARGEST_NORMALIZED_VECTOR_COMPONENT) {
           int j = (i + 1) % 3;
           int k = (j + 1) % 3;
           _perpAxis[i] = - _axis[j];
           _perpAxis[j] = component;
           _perpAxis[k] = 0.0f;
           _perpAxis = glm::normalize(_perpAxis);
           break;
       }
    }
}

void ElbowConstraint::setAngleLimits(float minAngle, float maxAngle) {
    // NOTE: min/maxAngle angles should be in the range [-PI, PI]
    _minAngle = glm::min(minAngle, maxAngle);
    _maxAngle = glm::max(minAngle, maxAngle);
}

bool ElbowConstraint::apply(glm::quat& rotation) const {
    // decompose the rotation into swing and twist
    // NOTE: rotation = postRotation * referenceRotation
    glm::quat postRotation = rotation * glm::inverse(_referenceRotation);
    glm::quat swingRotation, twistRotation;
    swingTwistDecomposition(postRotation, _axis, swingRotation, twistRotation);
    // NOTE: postRotation = swingRotation * twistRotation

    // compute twistAngle
    float twistAngle = 2.0f * acosf(fabsf(twistRotation.w));
    glm::vec3 twisted = twistRotation * _perpAxis;
    twistAngle *= copysignf(1.0f, glm::dot(glm::cross(_perpAxis, twisted), _axis));

    // clamp twistAngle
    float clampedTwistAngle = glm::clamp(twistAngle, _minAngle, _maxAngle);
    bool twistWasClamped = (twistAngle != clampedTwistAngle);

    // update rotation
    const float MIN_SWING_REAL_PART = 0.99999f;
    if (twistWasClamped || fabsf(swingRotation.w) < MIN_SWING_REAL_PART) {
        if (twistWasClamped) {
            twistRotation = glm::angleAxis(clampedTwistAngle, _axis);
        }
        // we discard all swing and only keep twist
        rotation = twistRotation * _referenceRotation;
        return true;
    }
    return false;
}

glm::quat ElbowConstraint::computeCenterRotation() const {
    const size_t NUM_LIMITS = 2;
    glm::quat limits[NUM_LIMITS];
    limits[0] = glm::angleAxis(_minAngle, _axis) * _referenceRotation;
    limits[1] = glm::angleAxis(_maxAngle, _axis) * _referenceRotation;
    return averageQuats(NUM_LIMITS, limits);
}
