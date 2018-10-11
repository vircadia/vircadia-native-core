//
//  Created by Anthony Thibault 2018/11/09
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "AccelerationLimiterFilter.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include "../../UserInputMapper.h"
#include "../../Input.h"
#include <DependencyManager.h>
#include <QDebug>
#include <StreamUtils.h>

static const QString JSON_ROTATION_ACCELERATION_LIMIT = QStringLiteral("rotationAccelerationLimit");
static const QString JSON_TRANSLATION_ACCELERATION_LIMIT = QStringLiteral("translationAccelerationLimit");
static const QString JSON_TRANSLATION_SNAP_THRESHOLD = QStringLiteral("translationSnapThreshold");
static const QString JSON_ROTATION_SNAP_THRESHOLD = QStringLiteral("rotationSnapThreshold");

static glm::vec3 angularVelFromDeltaRot(const glm::quat& deltaQ, float dt) {
    // Measure the angular velocity of a delta rotation quaternion by using quaternion logarithm.
    // The logarithm of a unit quternion returns the axis of rotation with a length of one half the angle of rotation in the imaginary part.
    // The real part will be 0.  Then we multiply it by 2 / dt. turning it into the angular velocity, (except for the extra w = 0 part).
    glm::quat omegaQ((2.0f / dt) * glm::log(deltaQ));
    return glm::vec3(omegaQ.x, omegaQ.y, omegaQ.z);
}

static glm::quat deltaRotFromAngularVel(const glm::vec3& omega, float dt) {
    // Convert angular velocity into a delta quaternion by using quaternion exponent.
    // The exponent of quaternion will return a delta rotation around the axis of the imaginary part, by twice the angle as determined by the length of that imaginary part.
    // It is the inverse of the logarithm step in angularVelFromDeltaRot
    glm::quat omegaQ(0.0f, omega.x, omega.y, omega.z);
    return glm::exp((dt / 2.0f) * omegaQ);
}

static glm::vec3 filterTranslation(const glm::vec3& x0, const glm::vec3& x1, const glm::vec3& x2, const glm::vec3& x3,
                                   float dt, float accLimit, float snapThreshold) {

    // measure the linear velocities of this step and the previoius step
    glm::vec3 v1 = (x3 - x1) / (2.0f * dt);
    glm::vec3 v0 = (x2 - x0) / (2.0f * dt);

    // compute the acceleration
    const glm::vec3 a = (v1 - v0) / dt;

    // clamp the acceleration if it is over the limit
    float aLen = glm::length(a);

    // pick limit based on if we are moving faster then our target
    float distToTarget = glm::length(x3 - x2);
    if (aLen > accLimit && distToTarget > snapThreshold) {
        // Solve for a new `v1`, such that `a` does not exceed `aLimit`
        // This combines two steps:
        // 1) Computing a limited accelration in the direction of `a`, but with a magnitute of `aLimit`:
        //    `newA = a * (aLimit / aLen)`
        // 2) Computing new `v1`
        //    `v1 = newA * dt + v0`
        // We combine the scalars from step 1 and step 2 into a single term to avoid having to do multiple scalar-vec3 multiplies.
        v1 = a * ((accLimit * dt) / aLen) + v0;

        // apply limited v1 to compute filtered x3
        return v1 * dt + x2;
    } else {
        // did not exceed limit, no filtering necesary
        return x3;
    }
}

static glm::quat filterRotation(const glm::quat& q0In, const glm::quat& q1In, const glm::quat& q2In, const glm::quat& q3In,
                                float dt, float accLimit, float snapThreshold) {

    // ensure quaternions have the same polarity
    glm::quat q0 = q0In;
    glm::quat q1 = glm::dot(q0In, q1In) < 0.0f ? -q1In : q1In;
    glm::quat q2 = glm::dot(q1In, q2In) < 0.0f ? -q2In : q2In;
    glm::quat q3 = glm::dot(q2In, q3In) < 0.0f ? -q3In : q3In;

    // measure the angular velocities of this step and the previous step
    glm::vec3 w1 = angularVelFromDeltaRot(q3 * glm::inverse(q1), 2.0f * dt);
    glm::vec3 w0 = angularVelFromDeltaRot(q2 * glm::inverse(q0), 2.0f * dt);

    const glm::vec3 a = (w1 - w0) / dt;
    float aLen = glm::length(a);

    // clamp the acceleration if it is over the limit
    float angleToTarget = glm::angle(q3 * glm::inverse(q2));
    if (aLen > accLimit && angleToTarget > snapThreshold) {
        // solve for a new w1, such that a does not exceed the accLimit
        w1 = a * ((accLimit * dt) / aLen) + w0;

        // apply limited w1 to compute filtered q3
        return deltaRotFromAngularVel(w1, dt) * q2;
    } else {
        // did not exceed limit, no filtering necesary
        return q3;
    }
}

namespace controller {

    Pose AccelerationLimiterFilter::apply(Pose value) const {

        if (value.isValid()) {

            // to perform filtering in sensor space, we need to compute the transformations.
            auto userInputMapper = DependencyManager::get<UserInputMapper>();
            const InputCalibrationData calibrationData = userInputMapper->getInputCalibrationData();
            glm::mat4 sensorToAvatarMat = glm::inverse(calibrationData.avatarMat) * calibrationData.sensorToWorldMat;
            glm::mat4 avatarToSensorMat = glm::inverse(calibrationData.sensorToWorldMat) * calibrationData.avatarMat;

            // transform pose into sensor space.
            Pose sensorValue = value.transform(avatarToSensorMat);

            if (_prevValid) {

                const float DELTA_TIME = 0.01111111f;

                glm::vec3 unfilteredTranslation = sensorValue.translation;
                sensorValue.translation = filterTranslation(_prevPos[0], _prevPos[1], _prevPos[2], sensorValue.translation,
                                                            DELTA_TIME, _translationAccelerationLimit, _translationSnapThreshold);
                glm::quat unfilteredRot = sensorValue.rotation;
                sensorValue.rotation = filterRotation(_prevRot[0], _prevRot[1], _prevRot[2], sensorValue.rotation,
                                                      DELTA_TIME, _rotationAccelerationLimit, _rotationSnapThreshold);

                // remember previous values.
                _prevPos[0] = _prevPos[1];
                _prevPos[1] = _prevPos[2];
                _prevPos[2] = sensorValue.translation;
                _prevRot[0] = _prevRot[1];
                _prevRot[1] = _prevRot[2];
                _prevRot[2] = sensorValue.rotation;

                _unfilteredPrevPos[0] = _unfilteredPrevPos[1];
                _unfilteredPrevPos[1] = _unfilteredPrevPos[2];
                _unfilteredPrevPos[2] = unfilteredTranslation;
                _unfilteredPrevRot[0] = _unfilteredPrevRot[1];
                _unfilteredPrevRot[1] = _unfilteredPrevRot[2];
                _unfilteredPrevRot[2] = unfilteredRot;

                // transform back into avatar space
                return sensorValue.transform(sensorToAvatarMat);
            } else {
                // initialize previous values with the current sample.
                _prevPos[0] = sensorValue.translation;
                _prevPos[1] = sensorValue.translation;
                _prevPos[2] = sensorValue.translation;
                _prevRot[0] = sensorValue.rotation;
                _prevRot[1] = sensorValue.rotation;
                _prevRot[2] = sensorValue.rotation;

                _unfilteredPrevPos[0] = sensorValue.translation;
                _unfilteredPrevPos[1] = sensorValue.translation;
                _unfilteredPrevPos[2] = sensorValue.translation;
                _unfilteredPrevRot[0] = sensorValue.rotation;
                _unfilteredPrevRot[1] = sensorValue.rotation;
                _unfilteredPrevRot[2] = sensorValue.rotation;

                _prevValid = true;

                // no previous value to smooth with, so return value unchanged
                return value;
            }
        } else {
            // mark previous poses as invalid.
            _prevValid = false;

            // return invalid value unchanged
            return value;
        }
    }

    bool AccelerationLimiterFilter::parseParameters(const QJsonValue& parameters) {
        if (parameters.isObject()) {
            auto obj = parameters.toObject();
            if (obj.contains(JSON_ROTATION_ACCELERATION_LIMIT) && obj.contains(JSON_TRANSLATION_ACCELERATION_LIMIT) &&
                obj.contains(JSON_ROTATION_SNAP_THRESHOLD) && obj.contains(JSON_TRANSLATION_SNAP_THRESHOLD)) {
                _rotationAccelerationLimit = (float)obj[JSON_ROTATION_ACCELERATION_LIMIT].toDouble();
                _translationAccelerationLimit = (float)obj[JSON_TRANSLATION_ACCELERATION_LIMIT].toDouble();
                _rotationSnapThreshold = (float)obj[JSON_ROTATION_SNAP_THRESHOLD].toDouble();
                _translationSnapThreshold = (float)obj[JSON_TRANSLATION_SNAP_THRESHOLD].toDouble();
                return true;
            }
        }
        return false;
    }

}
