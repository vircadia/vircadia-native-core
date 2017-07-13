//
//  LaserPointerScriptingInterface.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LaserPointerScriptingInterface.h"

#include <QVariant>
#include "RegisteredMetaTypes.h"
#include "GLMHelpers.h"

LaserPointerScriptingInterface* LaserPointerScriptingInterface::getInstance() {
    static LaserPointerScriptingInterface instance;
    return &instance;
}

uint32_t LaserPointerScriptingInterface::createLaserPointer(const QVariant& properties) {
    QVariantMap propertyMap = properties.toMap();

    if (propertyMap["joint"].isValid()) {
        QString jointName = propertyMap["joint"].toString();

        // x = upward, y = forward, z = lateral
        glm::vec3 posOffset = Vectors::ZERO;
        if (propertyMap["posOffset"].isValid()) {
            posOffset = vec3FromVariant(propertyMap["posOffset"]);
        }

        glm::vec3 dirOffset = Vectors::UP;
        if (propertyMap["dirOffset"].isValid()) {
            dirOffset = vec3FromVariant(propertyMap["dirOffset"]);
        }

        uint16_t filter = 0;
        if (propertyMap["filter"].isValid()) {
            filter = propertyMap["filter"].toUInt();
        }

        float maxDistance = 0.0f;
        if (propertyMap["maxDistance"].isValid()) {
            maxDistance = propertyMap["maxDistance"].toFloat();
        }

        bool enabled = false;
        if (propertyMap["enabled"].isValid()) {
            enabled = propertyMap["enabled"].toBool();
        }

        // TODO:
        // handle render state properties

        return LaserPointerManager::getInstance().createLaserPointer(jointName, posOffset, dirOffset, filter, maxDistance, enabled);
    } else {
        return 0;
    }
}