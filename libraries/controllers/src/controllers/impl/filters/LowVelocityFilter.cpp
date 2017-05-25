//
//  Created by Dante Ruiz 2017/05/15
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "LowVelocityFilter.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include "../../UserInputMapper.h"
#include "../../Input.h"
#include <DependencyManager.h>

static const QString JSON_ROTATION = QStringLiteral("rotation");
static const QString JSON_TRANSLATION = QStringLiteral("translation");
namespace controller {

    Pose LowVelocityFilter::apply(Pose newPose) const {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        const InputCalibrationData calibrationData = userInputMapper->getInputCalibrationData();
        glm::mat4 sensorToAvatarMat = glm::inverse(calibrationData.avatarMat) * calibrationData.sensorToWorldMat;
        glm::mat4 avatarToSensorMat = glm::inverse(calibrationData.sensorToWorldMat) * calibrationData.avatarMat;
        Pose finalPose = newPose;
        if (finalPose.isValid() && _oldPose.isValid()) {
            Pose sensorPose = finalPose.transform(avatarToSensorMat);
            Pose lastPose = _oldPose;

            float rotationFilter = glm::clamp(1.0f - (glm::length(lastPose.getVelocity() / _rotationConstant)), 0.0f, 1.0f);
            float translationFilter = glm::clamp(1.0f - (glm::length(lastPose.getVelocity() / _translationConstant)), 0.0f, 1.0f);
            sensorPose.translation = lastPose.getTranslation() * translationFilter + sensorPose.getTranslation() * (1.0f - translationFilter);
            sensorPose.rotation = safeMix(lastPose.getRotation(), sensorPose.getRotation(), 1.0f - rotationFilter);

            finalPose = sensorPose.transform(sensorToAvatarMat);
        }
        _oldPose = finalPose.transform(avatarToSensorMat);
        return finalPose;
    }

    bool LowVelocityFilter::parseParameters(const QJsonValue& parameters) {

        if (parameters.isObject()) {
            auto obj = parameters.toObject();
            if (obj.contains(JSON_ROTATION) && obj.contains(JSON_TRANSLATION)) {
                _rotationConstant = obj[JSON_ROTATION].toDouble();
                _translationConstant = obj[JSON_TRANSLATION].toDouble();
                return true;
            }
        }
        return false;
    }

}
