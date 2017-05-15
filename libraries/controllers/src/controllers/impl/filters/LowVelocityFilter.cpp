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

static const QString JSON_ROTATION = QStringLiteral("rotation");
static const QString JSON_TRANSLATION = QStringLiteral("translation");
namespace controller {

    Pose LowVelocityFilter::apply(Pose newPose) const {
        Pose finalPose = newPose;
        if (finalPose.isValid()) {
            float rotationFilter = glm::clamp(1.0f - (glm::length(_oldPose.getVelocity() / _rotationConstant)), 0.0f, 1.0f);
            float translationFilter = glm::clamp(1.0f - (glm::length(_oldPose.getVelocity() / _translationConstant)), 0.0f, 1.0f);
            finalPose.translation = _oldPose.getTranslation() * translationFilter + newPose.getTranslation() * (1.0f - translationFilter);
            finalPose.rotation = safeMix(_oldPose.getRotation(), newPose.getRotation(), 1.0f - rotationFilter);
        }
        _oldPose = finalPose;
        return finalPose;
    }

    bool LowVelocityFilter::parseParameters(const QJsonValue& parameters) {

        if (parameters.isObject()) {
            auto obj = parameters.toObject();
            if (obj.contains(JSON_ROTATION) && obj.contains(JSON_TRANSLATION)) {
                _rotationConstant = obj[JSON_ROTATION].toDouble();
                _translationConstant = obj[JSON_TRANSLATION].toDouble();
                qDebug() << "--------->Successfully parsed low velocity filter";
                return true;
            }
        }
        qDebug() << "--------->failed parsed low velocity filter";
        return false;
    }
    
}
