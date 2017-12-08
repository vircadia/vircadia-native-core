//
//  Created by Anthony Thibault 2017/12/07
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "ExponentialSmoothingFilter.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include "../../UserInputMapper.h"
#include "../../Input.h"
#include <DependencyManager.h>

static const QString JSON_ROTATION = QStringLiteral("rotation");
static const QString JSON_TRANSLATION = QStringLiteral("translation");
namespace controller {

    Pose ExponentialSmoothingFilter::apply(Pose value) const {

        // to perform filtering in sensor space, we need to compute the transformations.
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        const InputCalibrationData calibrationData = userInputMapper->getInputCalibrationData();
        glm::mat4 sensorToAvatarMat = glm::inverse(calibrationData.avatarMat) * calibrationData.sensorToWorldMat;
        glm::mat4 avatarToSensorMat = glm::inverse(calibrationData.sensorToWorldMat) * calibrationData.avatarMat;

        // transform pose into sensor space.
        Pose sensorValue = value.transform(avatarToSensorMat);

        if (value.isValid() && _oldSensorValue.isValid()) {

            // exponential smoothing filter
            sensorValue.translation = _translationConstant * sensorValue.getTranslation() + (1.0f - _translationConstant) * _oldSensorValue.getTranslation();
            sensorValue.rotation = safeMix(sensorValue.getRotation(), _oldSensorValue.getRotation(), _rotationConstant);

            _oldSensorValue = sensorValue;

            // transform back into avatar space
            return sensorValue.transform(sensorToAvatarMat);
        } else {
            _oldSensorValue = sensorValue;
            return value;
        }
    }

    bool ExponentialSmoothingFilter::parseParameters(const QJsonValue& parameters) {

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
