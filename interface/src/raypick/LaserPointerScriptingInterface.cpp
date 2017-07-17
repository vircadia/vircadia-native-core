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

#include "Application.h"

LaserPointerScriptingInterface* LaserPointerScriptingInterface::getInstance() {
    static LaserPointerScriptingInterface instance;
    return &instance;
}

uint32_t LaserPointerScriptingInterface::createLaserPointer(const QVariant& properties) {
    QVariantMap propertyMap = properties.toMap();

    uint16_t filter = 0;
    if (propertyMap["filter"].isValid()) {
        filter = propertyMap["filter"].toUInt();
    }

    float maxDistance = 0.0f;
    if (propertyMap["maxDistance"].isValid()) {
        maxDistance = propertyMap["maxDistance"].toFloat();
    }

    bool faceAvatar = false;
    if (propertyMap["faceAvatar"].isValid()) {
        faceAvatar = propertyMap["faceAvatar"].toBool();
    }

    bool centerEndY = true;
    if (propertyMap["centerEndY"].isValid()) {
        centerEndY = propertyMap["centerEndY"].toBool();
    }

    bool enabled = false;
    if (propertyMap["enabled"].isValid()) {
        enabled = propertyMap["enabled"].toBool();
    }

    QHash<QString, RenderState> renderStates;
    if (propertyMap["renderStates"].isValid()) {
        QList<QVariant> renderStateVariants = propertyMap["renderStates"].toList();
        for (QVariant& renderStateVariant : renderStateVariants) {
            if (renderStateVariant.isValid()) {
                QVariantMap renderStateMap = renderStateVariant.toMap();
                if (renderStateMap["name"].isValid()) {
                    QString name = renderStateMap["name"].toString();

                    QUuid startID;
                    if (renderStateMap["start"].isValid()) {
                        QVariantMap startMap = renderStateMap["start"].toMap();
                        if (startMap["type"].isValid()) {
                            startID = qApp->getOverlays().addOverlay(startMap["type"].toString(), startMap);
                        }
                    }

                    QUuid pathID;
                    if (renderStateMap["path"].isValid()) {
                        QVariantMap pathMap = renderStateMap["path"].toMap();
                        // right now paths must be line3ds
                        if (pathMap["type"].isValid() && pathMap["type"].toString() == "line3d") {
                            pathID = qApp->getOverlays().addOverlay(pathMap["type"].toString(), pathMap);
                        }
                    }

                    QUuid endID;
                    if (renderStateMap["end"].isValid()) {
                        QVariantMap endMap = renderStateMap["end"].toMap();
                        if (endMap["type"].isValid()) {
                            endID = qApp->getOverlays().addOverlay(endMap["type"].toString(), endMap);
                        }
                    }

                    renderStates[name] = RenderState(startID, pathID, endID);
                }
            }
        }
    }

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

        return LaserPointerManager::getInstance().createLaserPointer(jointName, posOffset, dirOffset, filter, maxDistance, renderStates, faceAvatar, centerEndY, enabled);
    } else if (propertyMap["position"].isValid()) {
        glm::vec3 position = vec3FromVariant(propertyMap["position"]);

        glm::vec3 direction = -Vectors::UP;
        if (propertyMap["direction"].isValid()) {
            direction = vec3FromVariant(propertyMap["direction"]);
        }

        return LaserPointerManager::getInstance().createLaserPointer(position, direction, filter, maxDistance, renderStates, faceAvatar, centerEndY, enabled);
    }

    return 0;
}