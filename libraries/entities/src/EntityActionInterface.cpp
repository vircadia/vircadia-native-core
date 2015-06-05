//
//  EntityActionInterface.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-6-4
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityItem.h"

#include "EntityActionInterface.h"


EntityActionType EntityActionInterface::actionTypeFromString(QString actionTypeString) {
    QString normalizedActionTypeString = actionTypeString.toLower().remove('-').remove('_');
    if (normalizedActionTypeString == "none") {
        return ACTION_TYPE_NONE;
    }
    if (normalizedActionTypeString == "pulltopoint") {
        return ACTION_TYPE_PULL_TO_POINT;
    }

    qDebug() << "Warning -- EntityActionInterface::actionTypeFromString got unknown action-type name" << actionTypeString;
    return ACTION_TYPE_NONE;
}

QString EntityActionInterface::actionTypeToString(EntityActionType actionType) {
    switch(actionType) {
        case ACTION_TYPE_NONE:
            return "none";
        case ACTION_TYPE_PULL_TO_POINT:
            return "pullToPoint";
    }
    assert(false);
    return "none";
}

glm::vec3 EntityActionInterface::extractVec3Argument(QString objectName, QVariantMap arguments,
                                                     QString argumentName, bool& ok) {
    if (!arguments.contains(argumentName)) {
        qDebug() << objectName << "requires argument:" << argumentName;
        ok = false;
        return vec3();
    }

    QVariant resultV = arguments[argumentName];
    if (resultV.type() != (QVariant::Type) QMetaType::QVariantMap) {
        qDebug() << objectName << "argument" << argumentName << "must be a map";
        ok = false;
        return vec3();
    }

    QVariantMap resultVM = resultV.toMap();
    if (!resultVM.contains("x") || !resultVM.contains("y") || !resultVM.contains("z")) {
        qDebug() << objectName << "argument" << argumentName << "must be a map with keys of x, y, z";
        ok = false;
        return vec3();
    }

    QVariant xV = resultVM["x"];
    QVariant yV = resultVM["y"];
    QVariant zV = resultVM["z"];

    bool xOk = true;
    bool yOk = true;
    bool zOk = true;
    float x = xV.toFloat(&xOk);
    float y = yV.toFloat(&yOk);
    float z = zV.toFloat(&zOk);
    if (!xOk || !yOk || !zOk) {
        qDebug() << objectName << "argument" << argumentName << "must be a map with keys of x, y, z and values of type float.";
        ok = false;
        return vec3();
    }

    return vec3(x, y, z);
}


float EntityActionInterface::extractFloatArgument(QString objectName, QVariantMap arguments,
                                                  QString argumentName, bool& ok) {
    if (!arguments.contains(argumentName)) {
        qDebug() << objectName << "requires argument:" << argumentName;
        ok = false;
        return 0.0f;
    }

    QVariant vV = arguments[argumentName];
    bool vOk = true;
    float v = vV.toFloat(&vOk);

    if (!vOk) {
        ok = false;
        return 0.0f;
    }

    return v;
}
