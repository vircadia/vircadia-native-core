//
//  FloatingUIPanel.cpp
//  interface/src/ui/overlays
//
//  Created by Zander Otavka on 7/2/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FloatingUIPanel.h"

#include <QVariant>
#include <RegisteredMetaTypes.h>

#include "Application.h"


glm::quat FloatingUIPanel::getOffsetRotation() const {
    if (getActualOffsetRotation() == glm::quat(0, 0, 0, 0)) {
        return Application::getInstance()->getCamera()->getOrientation() * glm::quat(0, 0, 1, 0);
    }
    return getActualOffsetRotation();
}

QScriptValue FloatingUIPanel::getProperty(const QString &property) {
    if (property == "offsetPosition") {
        return vec3toScriptValue(_scriptEngine, getOffsetPosition());
    }
    if (property == "offsetRotation") {
        return quatToScriptValue(_scriptEngine, getActualOffsetRotation());
    }
    if (property == "facingRotation") {
        return quatToScriptValue(_scriptEngine, getFacingRotation());
    }

    return QScriptValue();
}

void FloatingUIPanel::setProperties(const QScriptValue &properties) {
    QScriptValue offsetPosition = properties.property("offsetPosition");
    if (offsetPosition.isValid()) {
        QScriptValue x = offsetPosition.property("x");
        QScriptValue y = offsetPosition.property("y");
        QScriptValue z = offsetPosition.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newPosition;
            newPosition.x = x.toVariant().toFloat();
            newPosition.y = y.toVariant().toFloat();
            newPosition.z = z.toVariant().toFloat();
            setOffsetPosition(newPosition);
        }
    }

    QScriptValue offsetRotation = properties.property("offsetRotation");
    if (offsetRotation.isValid()) {
        QScriptValue x = offsetRotation.property("x");
        QScriptValue y = offsetRotation.property("y");
        QScriptValue z = offsetRotation.property("z");
        QScriptValue w = offsetRotation.property("w");

        if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) {
            glm::quat newRotation;
            newRotation.x = x.toVariant().toFloat();
            newRotation.y = y.toVariant().toFloat();
            newRotation.z = z.toVariant().toFloat();
            newRotation.w = w.toVariant().toFloat();
            setOffsetRotation(newRotation);
        }
    }

    QScriptValue facingRotation = properties.property("facingRotation");
    if (offsetRotation.isValid()) {
        QScriptValue x = facingRotation.property("x");
        QScriptValue y = facingRotation.property("y");
        QScriptValue z = facingRotation.property("z");
        QScriptValue w = facingRotation.property("w");

        if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) {
            glm::quat newRotation;
            newRotation.x = x.toVariant().toFloat();
            newRotation.y = y.toVariant().toFloat();
            newRotation.z = z.toVariant().toFloat();
            newRotation.w = w.toVariant().toFloat();
            setFacingRotation(newRotation);
        }
    }
}
