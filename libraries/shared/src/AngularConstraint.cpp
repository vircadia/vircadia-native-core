//
//  AngularConstraint.cpp
//  interface/src/renderer
//
//  Created by Andrew Meadows on 2014.05.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/norm.hpp>

#include "AngularConstraint.h"
#include "GLMHelpers.h"

// helper function
/// \param angle radian angle to be clamped within angleMin and angleMax
/// \param angleMin minimum value
/// \param angleMax maximum value
/// \return value between minAngle and maxAngle closest to angle
float clampAngle(float angle, float angleMin, float angleMax) {
    float minDistance = angle - angleMin;
    float maxDistance = angle - angleMax;
    if (maxDistance > 0.0f) {
        minDistance = glm::min(minDistance, angleMin + TWO_PI - angle);
        angle = (minDistance < maxDistance) ? angleMin : angleMax;
    } else if (minDistance < 0.0f) {
        maxDistance = glm::max(maxDistance, angleMax - TWO_PI - angle);
        angle = (minDistance > maxDistance) ? angleMin : angleMax;
    }
    return angle;
}

// static 
AngularConstraint* AngularConstraint::newAngularConstraint(const glm::vec3& minAngles, const glm::vec3& maxAngles) {
    float minDistance2 = glm::distance2(minAngles, glm::vec3(-PI, -PI, -PI));
    float maxDistance2 = glm::distance2(maxAngles, glm::vec3(PI, PI, PI));
    if (minDistance2 < EPSILON && maxDistance2 < EPSILON) {
        // no constraint
        return NULL;
    }
    // count the zero length elements
    glm::vec3 rangeAngles = maxAngles - minAngles;
    int pivotIndex = -1;
    int numZeroes = 0;
    for (int i = 0; i < 3; ++i) {
        if (rangeAngles[i] < EPSILON) {
            ++numZeroes;
        } else {
            pivotIndex = i;
        }
    }
    if (numZeroes == 2) {
        // this is a hinge
        int forwardIndex = (pivotIndex + 1) % 3;
        glm::vec3 forwardAxis(0.0f);
        forwardAxis[forwardIndex] = 1.0f;
        glm::vec3 rotationAxis(0.0f);
        rotationAxis[pivotIndex] = 1.0f;
        return new HingeConstraint(forwardAxis, rotationAxis, minAngles[pivotIndex], maxAngles[pivotIndex]);
    } else if (numZeroes == 0) {
        // approximate the angular limits with a cone roller
        // we assume the roll is about z
        glm::vec3 middleAngles = 0.5f * (maxAngles + minAngles);
        glm::quat yaw = glm::angleAxis(middleAngles[1], glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat pitch = glm::angleAxis(middleAngles[0], glm::vec3(1.0f, 0.0f, 0.0f));
        glm::vec3 coneAxis = pitch * yaw * glm::vec3(0.0f, 0.0f, 1.0f);
        // the coneAngle is half the average range of the two non-roll rotations
        glm::vec3 range = maxAngles - minAngles;
        float coneAngle = 0.25f * (range[0] + range[1]);
        return new ConeRollerConstraint(coneAngle, coneAxis, minAngles.z, maxAngles.z);
    }
    return NULL;
}

bool AngularConstraint::softClamp(glm::quat& targetRotation, const glm::quat& oldRotation, float mixFraction) {
    glm::quat clampedTarget = targetRotation;
    bool clamped = clamp(clampedTarget);
    if (clamped) {
        // check if oldRotation is also clamped
        glm::quat clampedOld = oldRotation;
        bool clamped2 = clamp(clampedOld);
        if (clamped2) {
            // oldRotation is already beyond the constraint
            // we clamp again midway between targetRotation and clamped oldPosition
            clampedTarget = glm::shortMix(clampedOld, targetRotation, mixFraction);
            // and then clamp that
            clamp(clampedTarget);
        }
        // finally we mix targetRotation with the clampedTarget
        targetRotation = glm::shortMix(clampedTarget, targetRotation, mixFraction);
    }
    return clamped;
}

HingeConstraint::HingeConstraint(const glm::vec3& forwardAxis, const glm::vec3& rotationAxis, float minAngle, float maxAngle)
        : _minAngle(minAngle), _maxAngle(maxAngle) {
    assert(_minAngle < _maxAngle);
    // we accept the rotationAxis direction
    assert(glm::length(rotationAxis) > EPSILON);
    _rotationAxis = glm::normalize(rotationAxis);
    // but we compute the final _forwardAxis
    glm::vec3 otherAxis = glm::cross(_rotationAxis, forwardAxis);
    assert(glm::length(otherAxis) > EPSILON);
    _forwardAxis = glm::normalize(glm::cross(otherAxis, _rotationAxis));
}

// virtual 
bool HingeConstraint::clamp(glm::quat& rotation) const {
    glm::vec3 forward = rotation * _forwardAxis;
    forward -= glm::dot(forward, _rotationAxis) * _rotationAxis;
    float length = glm::length(forward);
    if (length < EPSILON) {
        // infinite number of solutions ==> choose the middle of the contrained range
        rotation = glm::angleAxis(0.5f * (_minAngle + _maxAngle), _rotationAxis);
        return true;
    }
    forward /= length;
    float sign = (glm::dot(glm::cross(_forwardAxis, forward), _rotationAxis) > 0.0f ? 1.0f : -1.0f);
    //float angle = sign * acos(glm::dot(forward, _forwardAxis) / length);
    float angle = sign * acos(glm::dot(forward, _forwardAxis));
    glm::quat newRotation = glm::angleAxis(clampAngle(angle, _minAngle, _maxAngle), _rotationAxis);
    if (fabsf(1.0f - glm::dot(newRotation, rotation)) > EPSILON * EPSILON) {
        rotation = newRotation;
        return true;
    }
    return false;
}

bool HingeConstraint::softClamp(glm::quat& targetRotation, const glm::quat& oldRotation, float mixFraction) {
    // the hinge works best without a soft clamp
    return clamp(targetRotation);
}

ConeRollerConstraint::ConeRollerConstraint(float coneAngle, const glm::vec3& coneAxis, float minRoll, float maxRoll)
        : _coneAngle(coneAngle), _minRoll(minRoll), _maxRoll(maxRoll) {
    assert(_maxRoll >= _minRoll);
    float axisLength = glm::length(coneAxis);
    assert(axisLength > EPSILON);
    _coneAxis = coneAxis / axisLength;
}

// virtual 
bool ConeRollerConstraint::clamp(glm::quat& rotation) const {
    bool applied = false;
    glm::vec3 rotatedAxis = rotation * _coneAxis;
    glm::vec3 perpAxis = glm::cross(rotatedAxis, _coneAxis);
    float perpAxisLength = glm::length(perpAxis);
    if (perpAxisLength > EPSILON) {
        perpAxis /= perpAxisLength;
        // enforce the cone
        float angle = acosf(glm::dot(rotatedAxis, _coneAxis));
        if (angle > _coneAngle) {
            rotation = glm::angleAxis(angle - _coneAngle, perpAxis) * rotation;
            rotatedAxis = rotation * _coneAxis;
            applied = true;
        }
    } else {
        // the rotation is 100% roll
        // there is no obvious perp axis so we must pick one
        perpAxis = rotatedAxis;
        // find the first non-zero element:
        float iValue = 0.0f;
        int i = 0;
        for (i = 0; i < 3; ++i) {
            if (fabsf(perpAxis[i]) > EPSILON) {
                iValue = perpAxis[i];
                break;
            }
        }
        assert(i != 3);
        // swap or negate the next element
        int j = (i + 1) % 3;
        float jValue = perpAxis[j];
        if (fabsf(jValue - iValue) > EPSILON) {
            perpAxis[i] = jValue;
            perpAxis[j] = iValue;
        } else {
            perpAxis[i] = -iValue;
        }
        perpAxis = glm::cross(perpAxis, rotatedAxis);
        perpAxisLength = glm::length(perpAxis);
        assert(perpAxisLength > EPSILON);
        perpAxis /= perpAxisLength;
    }
    // measure the roll
    // NOTE: perpAxis is perpendicular to both _coneAxis and rotatedConeAxis, so we can 
    // rotate it again and we'll end up with an something that has only been rolled.
    glm::vec3 rolledPerpAxis = rotation * perpAxis;
    float sign = glm::dot(rotatedAxis, glm::cross(perpAxis, rolledPerpAxis)) > 0.0f ? 1.0f : -1.0f;
    float roll = sign * angleBetween(rolledPerpAxis, perpAxis);
    if (roll < _minRoll || roll > _maxRoll) {
        float clampedRoll = clampAngle(roll, _minRoll, _maxRoll);
        rotation = glm::normalize(glm::angleAxis(clampedRoll - roll, rotatedAxis) * rotation);
        applied = true;
    }
    return applied;
}


