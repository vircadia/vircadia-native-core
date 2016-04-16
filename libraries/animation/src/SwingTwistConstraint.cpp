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
#include <GLMHelpers.h>
#include <NumericalConstants.h>


const float MIN_MINDOT = -0.999f;
const float MAX_MINDOT = 1.0f;

const int LAST_CLAMP_LOW_BOUNDARY = -1;
const int LAST_CLAMP_NO_BOUNDARY = 0;
const int LAST_CLAMP_HIGH_BOUNDARY = 1;

SwingTwistConstraint::SwingLimitFunction::SwingLimitFunction() {
    _minDots.push_back(-1.0f);
    _minDots.push_back(-1.0f);

    _minDotIndexA = -1;
    _minDotIndexB = -1;
}

// In order to support the dynamic adjustment to swing limits we require
// that minDots have a minimum number of elements:
const int MIN_NUM_DOTS = 8;

void SwingTwistConstraint::SwingLimitFunction::setMinDots(const std::vector<float>& minDots) {
    int numDots = (int)minDots.size();
    _minDots.clear();
    if (numDots == 0) {
        // push multiple copies of MIN_MINDOT
        for (int i = 0; i < MIN_NUM_DOTS; ++i) {
            _minDots.push_back(MIN_MINDOT);
        }
        // push one more for cyclic boundary conditions
        _minDots.push_back(MIN_MINDOT);
    } else {
        // for minimal fidelity in the dynamic adjustment we expand the swing limit data until
        // we have enough data points
        int trueNumDots = numDots;
        int numFiller = 0;
        while(trueNumDots < MIN_NUM_DOTS) {
            numFiller++;
            trueNumDots += numDots;
        }
        _minDots.reserve(trueNumDots);

        for (int i = 0; i < numDots; ++i) {
            // push the next value
            _minDots.push_back(glm::clamp(minDots[i], MIN_MINDOT, MAX_MINDOT));

            if (numFiller > 0) {
                // compute endpoints of line segment
                float nearDot = glm::clamp(minDots[i], MIN_MINDOT, MAX_MINDOT);
                int k = (i + 1) % numDots;
                float farDot = glm::clamp(minDots[k], MIN_MINDOT, MAX_MINDOT);

                // fill the gap with interpolated values
                for (int j = 0; j < numFiller; ++j) {
                    float delta = (float)(j + 1) / float(numFiller + 1);
                    _minDots.push_back((1.0f - delta) * nearDot + delta * farDot);
                }
            }
        }
        // push the first value to the back to for cyclic boundary conditions
        _minDots.push_back(_minDots[0]);
    }
    _minDotIndexA = -1;
    _minDotIndexB = -1;
}

/// \param angle radian angle to update
/// \param minDotAdjustment minimum dot limit at that angle
void SwingTwistConstraint::SwingLimitFunction::dynamicallyAdjustMinDots(float theta, float minDotAdjustment) {
    // What does "dynamic adjustment" mean?
    //
    // Consider a limitFunction that looks like this:
    //
    //  1+
    //   |                valid space
    //   |
    //   +-----+-----+-----+-----+-----+-----+-----+-----+
    //   |
    //   |                invalid space
    //  0+------------------------------------------------
    //   0          pi/2         pi         3pi/2       2pi
    //   theta --->
    //
    // If we wanted to modify the envelope to accept a single invalid point X
    // then we would need to modify neighboring values A and B accordingly:
    //
    //  1+          adjustment for X at some thetaX
    //   |              |
    //   |              |
    //   +-----+.       V       .+-----+-----+-----+-----+
    //   |        -           -
    //   |         ' A--X--B '
    //  0+------------------------------------------------
    //   0          pi/2         pi         3pi/2       2pi
    //
    // The code below computes the values of A and B such that the line between them
    // passes through the point X, and we get reasonable interpolation for nearby values
    // of theta.  The old AB values are saved for later restore.

    if (_minDotIndexA > -1) {
        // retstore old values
        _minDots[_minDotIndexA] = _minDotA;
        _minDots[_minDotIndexB] = _minDotB;

        // handle cyclic boundary conditions
        int lastIndex = (int)_minDots.size() - 1;
        if (_minDotIndexA == 0) {
            _minDots[lastIndex] = _minDotA;
        } else if (_minDotIndexB == lastIndex) {
            _minDots[0] = _minDotB;
        }
    }

    // extract the positive normalized fractional part of the theta
    float integerPart;
    float normalizedAngle = modff(theta / TWO_PI, &integerPart);
    if (normalizedAngle < 0.0f) {
        normalizedAngle += 1.0f;
    }

    // interpolate between the two nearest points in the curve
    float delta = modff(normalizedAngle * (float)(_minDots.size() - 1), &integerPart);
    int indexA = (int)(integerPart);
    int indexB = (indexA + 1) % _minDots.size();
    float interpolatedDot = _minDots[indexA] * (1.0f - delta) + _minDots[indexB] * delta;

    if (minDotAdjustment < interpolatedDot) {
        // minDotAdjustment is outside the existing bounds so we must modify

        // remember the indices
        _minDotIndexA = indexA;
        _minDotIndexB = indexB;

        // save the old minDots
        _minDotA = _minDots[_minDotIndexA];
        _minDotB = _minDots[_minDotIndexB];

        // compute replacement values to _minDots that will provide a line segment
        // that passes through minDotAdjustment while balancing the distortion between A and B.
        // Note: the derivation of these formulae is left as an exercise to the reader.
        float twiceUndershoot = 2.0f * (minDotAdjustment - interpolatedDot);
        _minDots[_minDotIndexA] -= twiceUndershoot * (delta + 0.5f) * (delta - 1.0f);
        _minDots[_minDotIndexB] -= twiceUndershoot * delta * (delta - 1.5f);

        // handle cyclic boundary conditions
        int lastIndex = (int)_minDots.size() - 1;
        if (_minDotIndexA == 0) {
            _minDots[lastIndex] = _minDots[_minDotIndexA];
        } else if (_minDotIndexB == lastIndex) {
            _minDots[0] = _minDots[_minDotIndexB];
        }
    } else {
        // minDotAdjustment is inside bounds so there is nothing to do
        _minDotIndexA = -1;
        _minDotIndexB = -1;
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
        _maxTwist(PI),
        _lastTwistBoundary(LAST_CLAMP_NO_BOUNDARY)
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

    int numLimits = (int)swungDirections.size();
    limits.reserve(numLimits);

    // compute the limit pairs: <theta, minDot>
    for (int i = 0; i < numLimits; ++i) {
        float directionLength = glm::length(swungDirections[i]);
        if (directionLength > EPSILON) {
            glm::vec3 swingAxis = glm::cross(Vectors::UNIT_Y, swungDirections[i]);
            float theta = atan2f(-swingAxis.z, swingAxis.x);
            if (theta < 0.0f) {
                theta += TWO_PI;
            }
            limits.push_back(SwingLimitData(theta, swungDirections[i].y / directionLength));
        }
    }

    std::vector<float> minDots;
    numLimits = (int)limits.size();
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
        int rightIndex = 0;
        for (int i = 0; i < numLimits; ++i) {
            float theta = (float)i * deltaTheta;
            int leftIndex = (rightIndex - 1 + numLimits) % numLimits;
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

    _lastTwistBoundary = LAST_CLAMP_NO_BOUNDARY;
    _twistAdjusted = false;
}

// private
float SwingTwistConstraint::handleTwistBoundaryConditions(float twistAngle) const {
    // adjust measured twistAngle according to clamping history
    switch (_lastTwistBoundary) {
        case LAST_CLAMP_LOW_BOUNDARY:
            // clamp to min
            if (twistAngle > _maxTwist) {
                twistAngle -= TWO_PI;
            }
            break;
        case LAST_CLAMP_HIGH_BOUNDARY:
            // clamp to max
            if (twistAngle < _minTwist) {
                twistAngle += TWO_PI;
            }
            break;
        default: // LAST_CLAMP_NO_BOUNDARY
            // clamp to nearest boundary
            float midBoundary = 0.5f * (_maxTwist + _minTwist + TWO_PI);
            if (twistAngle > midBoundary) {
                // lower boundary is closer --> phase down one cycle
                twistAngle -= TWO_PI;
            } else if (twistAngle < midBoundary - TWO_PI) {
                // higher boundary is closer --> phase up one cycle
                twistAngle += TWO_PI;
            }
            break;
    }
    return twistAngle;
}

bool SwingTwistConstraint::apply(glm::quat& rotation) const {
    // decompose the rotation into first twist about yAxis, then swing about something perp
    // NOTE: rotation = postRotation * referenceRotation
    glm::quat postRotation = rotation * glm::inverse(_referenceRotation);
    glm::quat swingRotation, twistRotation;
    swingTwistDecomposition(postRotation, Vectors::UNIT_Y, swingRotation, twistRotation);
    // NOTE: postRotation = swingRotation * twistRotation

    // compute raw twistAngle
    float twistAngle = 2.0f * acosf(fabsf(twistRotation.w));
    glm::vec3 twistedX = twistRotation * Vectors::UNIT_X;
    twistAngle *= copysignf(1.0f, glm::dot(glm::cross(Vectors::UNIT_X, twistedX), Vectors::UNIT_Y));

    bool somethingClamped = false;
    if (_minTwist != _maxTwist) {
        // twist limits apply --> figure out which limit we're hitting, if any
        twistAngle = handleTwistBoundaryConditions(twistAngle);

        // clamp twistAngle
        float clampedTwistAngle = glm::clamp(twistAngle, _minTwist, _maxTwist);
        somethingClamped = (twistAngle != clampedTwistAngle);

        // remember twist's clamp boundary history
        if (somethingClamped) {
            _lastTwistBoundary = (twistAngle > clampedTwistAngle) ? LAST_CLAMP_HIGH_BOUNDARY : LAST_CLAMP_LOW_BOUNDARY;
            twistAngle = clampedTwistAngle;
        } else {
            _lastTwistBoundary = LAST_CLAMP_NO_BOUNDARY;
        }
    }

    // clamp the swing
    // The swingAxis is always perpendicular to the reference axis (yAxis in the constraint's frame).
    glm::vec3 swungY = swingRotation * Vectors::UNIT_Y;
    glm::vec3 swingAxis = glm::cross(Vectors::UNIT_Y, swungY);
    float axisLength = glm::length(swingAxis);
    if (axisLength > EPSILON) {
        // The limit of swing is a function of "theta" which can be computed from the swingAxis
        // (which is in the constraint's ZX plane).
        float theta = atan2f(-swingAxis.z, swingAxis.x);
        float minDot = _swingLimitFunction.getMinDot(theta);
        if (glm::dot(swungY, Vectors::UNIT_Y) < minDot) {
            // The swing limits are violated so we extract the angle from midDot and
            // use it to supply a new rotation.
            swingAxis /= axisLength;
            swingRotation = glm::angleAxis(acosf(minDot), swingAxis);
            somethingClamped = true;
        }
    }

    if (somethingClamped) {
        // update the rotation
        twistRotation = glm::angleAxis(twistAngle, Vectors::UNIT_Y);
        rotation = swingRotation * twistRotation * _referenceRotation;
        return true;
    }
    return false;
}

void SwingTwistConstraint::dynamicallyAdjustLimits(const glm::quat& rotation) {
    glm::quat postRotation = rotation * glm::inverse(_referenceRotation);
    glm::quat swingRotation, twistRotation;

    swingTwistDecomposition(postRotation, Vectors::UNIT_Y, swingRotation, twistRotation);

    { // adjust swing limits
        glm::vec3 swungY = swingRotation * Vectors::UNIT_Y;
        glm::vec3 swingAxis = glm::cross(Vectors::UNIT_Y, swungY);
        float theta = atan2f(-swingAxis.z, swingAxis.x);
        if (glm::isnan(theta)) {
            // atan2f() will only return NaN if either of its arguments is NaN, which can only
            // happen if we've been given a bad rotation.  Since a NaN value here could potentially
            // cause a crash (we use the value of theta to compute indices into a std::vector)
            // we specifically check for this case.
            theta = 0.0f;
            swungY.y = 1.0f;
        }
        _swingLimitFunction.dynamicallyAdjustMinDots(theta, swungY.y);
    }

    // restore twist limits
    if (_twistAdjusted) {
        _minTwist = _oldMinTwist;
        _maxTwist = _oldMaxTwist;
        _twistAdjusted = false;
    }

    if (_minTwist != _maxTwist) {
        // compute twistAngle
        float twistAngle = 2.0f * acosf(fabsf(twistRotation.w));
        glm::vec3 twistedX = twistRotation * Vectors::UNIT_X;
        twistAngle *= copysignf(1.0f, glm::dot(glm::cross(Vectors::UNIT_X, twistedX), Vectors::UNIT_Y));
        twistAngle = handleTwistBoundaryConditions(twistAngle);

        if (twistAngle < _minTwist || twistAngle > _maxTwist) {
            // expand twist limits
            _twistAdjusted = true;
            _oldMinTwist = _minTwist;
            _oldMaxTwist = _maxTwist;
            if (twistAngle < _minTwist) {
                _minTwist = twistAngle;
            } else if (twistAngle > _maxTwist) {
                _maxTwist = twistAngle;
            }
        }
    }
}

void SwingTwistConstraint::clearHistory() {
    _lastTwistBoundary = LAST_CLAMP_NO_BOUNDARY;
}
