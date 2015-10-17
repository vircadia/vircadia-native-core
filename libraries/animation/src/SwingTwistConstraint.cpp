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
    float minDot = glm::clamp(maxAngle, MIN_MINDOT, MAX_MINDOT);
    _minDots.push_back(minDot);
    // push the first value to the back to establish cyclic boundary conditions
    _minDots.push_back(minDot);
}

void SwingTwistConstraint::SwingLimitFunction::setMinDots(const std::vector<float>& minDots) {
    uint32_t numDots = minDots.size();
    _minDots.clear();
    if (numDots == 0) {
        // push two copies of MIN_MINDOT
        _minDots.push_back(MIN_MINDOT);
        _minDots.push_back(MIN_MINDOT);
    } else {
        _minDots.reserve(numDots);
        for (uint32_t i = 0; i < numDots; ++i) {
            _minDots.push_back(glm::clamp(minDots[i], MIN_MINDOT, MAX_MINDOT));
        }
        // push the first value to the back to establish cyclic boundary conditions
        _minDots.push_back(_minDots[0]);
    }
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
        RotationConstraint(),
        _swingLimitFunction(),
        _minTwist(-PI),
        _maxTwist(PI)
{
}

void SwingTwistConstraint::setSwingLimits(std::vector<float> minDots) {
    _swingLimitFunction.setMinDots(minDots);
}

void SwingTwistConstraint::setSwingLimits(const std::vector<glm::vec3>& swungDirections) {
    struct SwingLimitData {
        SwingLimitData() : _theta(0.0f), _minDot(1.0f) {}
        SwingLimitData(float theta, float minDot) : _theta(theta), _minDot(minDot) {}
        float _theta;
        float _minDot;
        bool operator<(const SwingLimitData& other) const { return _theta < other._theta; }
    };
    std::vector<SwingLimitData> limits;

    uint32_t numLimits = swungDirections.size();
    limits.reserve(numLimits);

    // compute the limit pairs: <theta, minDot>
    const glm::vec3 yAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    for (uint32_t i = 0; i < numLimits; ++i) {
        float directionLength = glm::length(swungDirections[i]);
        if (directionLength > EPSILON) {
            glm::vec3 swingAxis = glm::cross(yAxis, swungDirections[i]);
            float theta = atan2f(-swingAxis.z, swingAxis.x);
            if (theta < 0.0f) {
                theta += TWO_PI;
            }
            limits.push_back(SwingLimitData(theta, swungDirections[i].y / directionLength));
        }
    }

    std::vector<float> minDots;
    numLimits = limits.size();
    if (numLimits == 0) {
        // trivial case: nearly free constraint
        std::vector<float> minDots;
        _swingLimitFunction.setMinDots(minDots);
    } else if (numLimits == 1) {
        // trivial case: uniform conical constraint
        std::vector<float> minDots;
        minDots.push_back(limits[0]._minDot);
        _swingLimitFunction.setMinDots(minDots);
    } else {
        // interesting case: potentially non-uniform constraints

        // sort limits by theta
        std::sort(limits.begin(), limits.end());
    
        // extrapolate evenly distributed limits for fast lookup table
        float deltaTheta = TWO_PI / (float)(numLimits);
        uint32_t rightIndex = 0;
        for (uint32_t i = 0; i < numLimits; ++i) {
            float theta = (float)i * deltaTheta;
            uint32_t leftIndex = (rightIndex - 1) % numLimits;
            while (rightIndex < numLimits && theta > limits[rightIndex]._theta) {
                leftIndex = rightIndex++;
            }

            if (leftIndex == numLimits - 1) {
                // we straddle the boundary
                rightIndex = 0;
            }

            float rightTheta = limits[rightIndex]._theta;
            float leftTheta = limits[leftIndex]._theta;
            if (leftTheta > rightTheta) {
                // we straddle the boundary, but we need to figure out which way to stride
                // in order to keep theta between left and right
                if (leftTheta > theta) {
                    leftTheta -= TWO_PI;
                } else {
                    rightTheta += TWO_PI;
                }
            }

            // blend between the left and right minDots to get the value that corresponds to this theta
            float rightWeight = (theta - leftTheta) / (rightTheta - leftTheta);
            minDots.push_back((1.0f - rightWeight) * limits[leftIndex]._minDot + rightWeight * limits[rightIndex]._minDot);
        }
    }
    _swingLimitFunction.setMinDots(minDots);
}

void SwingTwistConstraint::setTwistLimits(float minTwist, float maxTwist) {
    // NOTE: min/maxTwist angles should be in the range [-PI, PI]
    _minTwist = glm::min(minTwist, maxTwist);
    _maxTwist = glm::max(minTwist, maxTwist);
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
    float clampedTwistAngle = glm::clamp(twistAngle, _minTwist, _maxTwist);
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
            // The swing limits are violated so we extract the angle from midDot and 
            // use it to supply a new rotation.
            swingAxis /= axisLength;
            swingRotation = glm::angleAxis(acosf(minDot), swingAxis);
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

