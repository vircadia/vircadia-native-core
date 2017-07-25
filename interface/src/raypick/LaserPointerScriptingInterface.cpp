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

QUuid LaserPointerScriptingInterface::createLaserPointer(const QVariant& properties) {
    QVariantMap propertyMap = properties.toMap();

    bool faceAvatar = false;
    if (propertyMap["faceAvatar"].isValid()) {
        faceAvatar = propertyMap["faceAvatar"].toBool();
    }

    bool centerEndY = true;
    if (propertyMap["centerEndY"].isValid()) {
        centerEndY = propertyMap["centerEndY"].toBool();
    }

    bool lockEnd = false;
    if (propertyMap["lockEnd"].isValid()) {
        lockEnd = propertyMap["lockEnd"].toBool();
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
                    renderStates[name] = buildRenderState(renderStateMap);
                }
            }
        }
    }

    return DependencyManager::get<LaserPointerManager>()->createLaserPointer(propertyMap, renderStates, faceAvatar, centerEndY, lockEnd, enabled);
}

void LaserPointerScriptingInterface::editRenderState(QUuid uid, const QString& renderState, const QVariant& properties) {
    QVariantMap propMap = properties.toMap();

    QVariant startProps;
    if (propMap["start"].isValid()) {
        startProps = propMap["start"];
    }

    QVariant pathProps;
    if (propMap["path"].isValid()) {
        pathProps = propMap["path"];
    }

    QVariant endProps;
    if (propMap["end"].isValid()) {
        endProps = propMap["end"];
    }

    DependencyManager::get<LaserPointerManager>()->editRenderState(uid, renderState, startProps, pathProps, endProps);
}

const RenderState LaserPointerScriptingInterface::buildRenderState(const QVariantMap& propMap) {
    QUuid startID;
    if (propMap["start"].isValid()) {
        QVariantMap startMap = propMap["start"].toMap();
        if (startMap["type"].isValid()) {
            startID = qApp->getOverlays().addOverlay(startMap["type"].toString(), startMap);
        }
    }

    QUuid pathID;
    if (propMap["path"].isValid()) {
        QVariantMap pathMap = propMap["path"].toMap();
        // right now paths must be line3ds
        if (pathMap["type"].isValid() && pathMap["type"].toString() == "line3d") {
            pathID = qApp->getOverlays().addOverlay(pathMap["type"].toString(), pathMap);
        }
    }

    QUuid endID;
    if (propMap["end"].isValid()) {
        QVariantMap endMap = propMap["end"].toMap();
        if (endMap["type"].isValid()) {
            endID = qApp->getOverlays().addOverlay(endMap["type"].toString(), endMap);
        }
    }

    return RenderState(startID, pathID, endID);
}