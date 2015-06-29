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
    if (normalizedActionTypeString == "offset") {
        return ACTION_TYPE_OFFSET;
    }
    if (normalizedActionTypeString == "spring") {
        return ACTION_TYPE_SPRING;
    }
    if (normalizedActionTypeString == "hold") {
        return ACTION_TYPE_HOLD;
    }

    qDebug() << "Warning -- EntityActionInterface::actionTypeFromString got unknown action-type name" << actionTypeString;
    return ACTION_TYPE_NONE;
}

QString EntityActionInterface::actionTypeToString(EntityActionType actionType) {
    switch(actionType) {
        case ACTION_TYPE_NONE:
            return "none";
        case ACTION_TYPE_OFFSET:
            return "offset";
        case ACTION_TYPE_SPRING:
            return "spring";
        case ACTION_TYPE_HOLD:
            return "hold";
    }
    assert(false);
    return "none";
}

glm::vec3 EntityActionInterface::extractVec3Argument(QString objectName, QVariantMap arguments,
                                                     QString argumentName, bool& ok, bool required) {
    if (!arguments.contains(argumentName)) {
        if (required) {
            qDebug() << objectName << "requires argument:" << argumentName;
        }
        ok = false;
        return glm::vec3();
    }

    QVariant resultV = arguments[argumentName];
    if (resultV.type() != (QVariant::Type) QMetaType::QVariantMap) {
        qDebug() << objectName << "argument" << argumentName << "must be a map";
        ok = false;
        return glm::vec3();
    }

    QVariantMap resultVM = resultV.toMap();
    if (!resultVM.contains("x") || !resultVM.contains("y") || !resultVM.contains("z")) {
        qDebug() << objectName << "argument" << argumentName << "must be a map with keys of x, y, z";
        ok = false;
        return glm::vec3();
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
        return glm::vec3();
    }

    return glm::vec3(x, y, z);
}

glm::quat EntityActionInterface::extractQuatArgument(QString objectName, QVariantMap arguments,
                                                     QString argumentName, bool& ok, bool required) {
    if (!arguments.contains(argumentName)) {
        if (required) {
            qDebug() << objectName << "requires argument:" << argumentName;
        }
        ok = false;
        return glm::quat();
    }

    QVariant resultV = arguments[argumentName];
    if (resultV.type() != (QVariant::Type) QMetaType::QVariantMap) {
        qDebug() << objectName << "argument" << argumentName << "must be a map, not" << resultV.typeName();
        ok = false;
        return glm::quat();
    }

    QVariantMap resultVM = resultV.toMap();
    if (!resultVM.contains("x") || !resultVM.contains("y") || !resultVM.contains("z")) {
        qDebug() << objectName << "argument" << argumentName << "must be a map with keys of x, y, z";
        ok = false;
        return glm::quat();
    }

    QVariant xV = resultVM["x"];
    QVariant yV = resultVM["y"];
    QVariant zV = resultVM["z"];
    QVariant wV = resultVM["w"];

    bool xOk = true;
    bool yOk = true;
    bool zOk = true;
    bool wOk = true;
    float x = xV.toFloat(&xOk);
    float y = yV.toFloat(&yOk);
    float z = zV.toFloat(&zOk);
    float w = wV.toFloat(&wOk);
    if (!xOk || !yOk || !zOk || !wOk) {
        qDebug() << objectName << "argument" << argumentName
                 << "must be a map with keys of x, y, z, w and values of type float.";
        ok = false;
        return glm::quat();
    }

    return glm::quat(w, x, y, z);
}

float EntityActionInterface::extractFloatArgument(QString objectName, QVariantMap arguments,
                                                  QString argumentName, bool& ok, bool required) {
    if (!arguments.contains(argumentName)) {
        if (required) {
            qDebug() << objectName << "requires argument:" << argumentName;
        }
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

QString EntityActionInterface::extractStringArgument(QString objectName, QVariantMap arguments,
                                                     QString argumentName, bool& ok, bool required) {
    if (!arguments.contains(argumentName)) {
        if (required) {
            qDebug() << objectName << "requires argument:" << argumentName;
        }
        ok = false;
        return "";
    }

    QVariant vV = arguments[argumentName];
    QString v = vV.toString();
    return v;
}
