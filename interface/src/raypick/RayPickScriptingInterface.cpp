//
//  RayPickScriptingInterface.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 8/15/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RayPickScriptingInterface.h"

#include <QVariant>
#include "GLMHelpers.h"
#include "Application.h"

QUuid RayPickScriptingInterface::createRayPick(const QVariant& properties) {
    QVariantMap propMap = properties.toMap();

    bool enabled = false;
    if (propMap["enabled"].isValid()) {
        enabled = propMap["enabled"].toBool();
    }

    RayPickFilter filter = RayPickFilter();
    if (propMap["filter"].isValid()) {
        filter = RayPickFilter(propMap["filter"].toUInt());
    }

    float maxDistance = 0.0f;
    if (propMap["maxDistance"].isValid()) {
        maxDistance = propMap["maxDistance"].toFloat();
    }

    if (propMap["joint"].isValid()) {
        std::string jointName = propMap["joint"].toString().toStdString();

        if (jointName != "Mouse") {
            // x = upward, y = forward, z = lateral
            glm::vec3 posOffset = Vectors::ZERO;
            if (propMap["posOffset"].isValid()) {
                posOffset = vec3FromVariant(propMap["posOffset"]);
            }

            glm::vec3 dirOffset = Vectors::UP;
            if (propMap["dirOffset"].isValid()) {
                dirOffset = vec3FromVariant(propMap["dirOffset"]);
            }

            return qApp->getRayPickManager().createRayPick(jointName, posOffset, dirOffset, filter, maxDistance, enabled);
        } else {
            return qApp->getRayPickManager().createRayPick(filter, maxDistance, enabled);
        }
    } else if (propMap["position"].isValid()) {
        glm::vec3 position = vec3FromVariant(propMap["position"]);

        glm::vec3 direction = -Vectors::UP;
        if (propMap["direction"].isValid()) {
            direction = vec3FromVariant(propMap["direction"]);
        }

        return qApp->getRayPickManager().createRayPick(position, direction, filter, maxDistance, enabled);
    }

    return QUuid();
}

void RayPickScriptingInterface::enableRayPick(QUuid uid) {
    qApp->getRayPickManager().enableRayPick(uid);
}

void RayPickScriptingInterface::disableRayPick(QUuid uid) {
    qApp->getRayPickManager().disableRayPick(uid);
}

void RayPickScriptingInterface::removeRayPick(QUuid uid) {
    qApp->getRayPickManager().removeRayPick(uid);
}

RayPickResult RayPickScriptingInterface::getPrevRayPickResult(QUuid uid) {
    return qApp->getRayPickManager().getPrevRayPickResult(uid);
}

void RayPickScriptingInterface::setPrecisionPicking(QUuid uid, const bool precisionPicking) {
    qApp->getRayPickManager().setPrecisionPicking(uid, precisionPicking);
}

void RayPickScriptingInterface::setIgnoreEntities(QUuid uid, const QScriptValue& ignoreEntities) {
    qApp->getRayPickManager().setIgnoreEntities(uid, ignoreEntities);
}

void RayPickScriptingInterface::setIncludeEntities(QUuid uid, const QScriptValue& includeEntities) {
    qApp->getRayPickManager().setIncludeEntities(uid, includeEntities);
}

void RayPickScriptingInterface::setIgnoreOverlays(QUuid uid, const QScriptValue& ignoreOverlays) {
    qApp->getRayPickManager().setIgnoreOverlays(uid, ignoreOverlays);
}

void RayPickScriptingInterface::setIncludeOverlays(QUuid uid, const QScriptValue& includeOverlays) {
    qApp->getRayPickManager().setIncludeOverlays(uid, includeOverlays);
}

void RayPickScriptingInterface::setIgnoreAvatars(QUuid uid, const QScriptValue& ignoreAvatars) {
    qApp->getRayPickManager().setIgnoreAvatars(uid, ignoreAvatars);
}

void RayPickScriptingInterface::setIncludeAvatars(QUuid uid, const QScriptValue& includeAvatars) {
    qApp->getRayPickManager().setIncludeAvatars(uid, includeAvatars);
}
