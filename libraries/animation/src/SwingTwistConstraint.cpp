//
//  SwingTwistConstraint.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SwingTwistConstraint.h"

#include <algorithm>
#include <math.h>

#include <GeometryUtil.h>
#include <NumericalConstants.h>


const float MIN_MINDOT = -0.999f;
const float MAX_MINDOT = 1.0f;

SwingTwistConstraint::SwingLimitFunction::SwingLimitFunction() {
    setCone(PI);
}

void SwingTwistConstraint::SwingLimitFunction::setCone(float maxAngle) {
    _minDots.clear();
    float minDot = std::max(MIN_MINDOT, std::min(cosf(maxAngle), MAX_MINDOT));
    _minDots.push_back(minDot);
    // push the first value to the back to establish cyclic boundary conditions
    _minDots.push_back(minDot);
}

void SwingTwistConstraint::SwingLimitFunction::setMinDots(const std::vector<float>& minDots) {
    int numDots = minDots.size();
    _minDots.clear();
    _minDots.reserve(numDots);
    for (int i = 0; i < numDots; ++i) {
        _minDots.push_back(std::max(MIN_MINDOT, std::min(minDots[i], MAX_MINDOT)));
    }
    // push the first value to the back to establish cyclic boundary conditions
    _minDots.push_back(_minDots[0]);
}

float SwingTwistConstraint::SwingLimitFunction::getMinDot(float theta) const {
    // extract the positive normalized fractional part of theta
    float integerPart;
    float normalizedTheta = modff(theta / TWO_PI, &integerPart);
    if (normalizedTheta < 0.0f) {
        normalizedTheta += 1.0f;
    }

    // interpolate between the two nearest points in the cycle
    float fractionPart = modff(normalizedTheta * (float)(_minDots.size() - 1), &integerPart);
    int i = (int)(integerPart);
    int j = (i + 1) % _minDots.size();
    return _minDots[i] * (1.0f - fractionPart) + _minDots[j] * fractionPart;
}

SwingTwistConstraint::SwingTwistConstraint() :
        _swingLimitFunction(),
        _referenceRotation(),
        _minTwist(-PI),
        _maxTwist(PI)
{
}

void SwingTwistConstraint::setSwingLimits(std::vector<float> minDots) {
    _swingLimitFunction.setMinDots(minDots);
}

void SwingTwistConstraint::setTwistLimits(float minTwist, float maxTwist) {
    // NOTE: min/maxTwist angles should be in the range [-PI, PI]
    _minTwist = std::min(minTwist, maxTwist);
    _maxTwist = std::max(minTwist, maxTwist);
}

bool SwingTwistConstraint::apply(glm::quat& rotation) const {

    // decompose the rotation into first twist about yAxis, then swing about something perp
    const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
    // NOTE: rotation = postRotation * referenceRotation
    glm::quat postRotation = rotation * glm::inverse(_referenceRotation);
    glm::quat swingRotation, twistRotation;
    swingTwistDecomposition(postRotation, yAxis, swingRotation, twistRotation);
    // NOTE: postRotation = swingRotation * twistRotation

    // compute twistAngle
    float twistAngle = 2.0f * acosf(fabsf(twistRotation.w));
    const glm::vec3 xAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 twistedX = twistRotation * xAxis;
    twistAngle *= copysignf(1.0f, glm::dot(glm::cross(xAxis, twistedX), yAxis));

    // clamp twistAngle
    float clampedTwistAngle = std::max(_minTwist, std::min(twistAngle, _maxTwist));
    bool twistWasClamped = (twistAngle != clampedTwistAngle);
    
    // clamp the swing
    // The swingAxis is always perpendicular to the reference axis (yAxis in the constraint's frame).
    glm::vec3 swungY = swingRotation * yAxis;
    glm::vec3 swingAxis = glm::cross(yAxis, swungY);
    bool swingWasClamped = false;
    float axisLength = glm::length(swingAxis);
    if (axisLength > EPSILON) {
        // The limit of swing is a function of "theta" which can be computed from the swingAxis
        // (which is in the constraint's ZX plane).
        float theta = atan2f(-swingAxis.z, swingAxis.x);
        float minDot = _swingLimitFunction.getMinDot(theta);
        if (glm::dot(swungY, yAxis) < minDot) {
            // The swing limits are violated so we use the maxAngle to supply a new rotation.
            float maxAngle = acosf(minDot);
            if (minDot < 0.0f) {
                maxAngle = PI - maxAngle;
            }
            swingAxis /= axisLength;
            swingRotation = glm::angleAxis(maxAngle, swingAxis);
            swingWasClamped = true;
        }
    }

    if (swingWasClamped || twistWasClamped) {
        twistRotation = glm::angleAxis(clampedTwistAngle, yAxis);
        rotation = swingRotation * twistRotation * _referenceRotation;
        return true;
    }
    return false;
}

