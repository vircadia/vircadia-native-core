//
//  PanelAttachable.cpp
//  hifi
//
//  Created by Zander Otavka on 7/15/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PanelAttachable.h"

#include <RegisteredMetaTypes.h>

#include "OverlayPanel.h"

bool PanelAttachable::getParentVisible() const {
    if (getParentPanel()) {
        return getParentPanel()->getVisible() && getParentPanel()->getParentVisible();
    } else {
        return true;
    }
}

void PanelAttachable::applyTransformTo(Transform& transform) {
    if (getParentPanel()) {
        transform.setTranslation(getParentPanel()->getComputedPosition());
        transform.setRotation(getParentPanel()->getComputedRotation());
        transform.postTranslate(getParentPanel()->getOffsetPosition());
        transform.postRotate(getParentPanel()->getOffsetRotation());
        transform.postTranslate(getOffsetPosition());
        transform.postRotate(getOffsetRotation());
    }
}

QScriptValue PanelAttachable::getProperty(QScriptEngine* scriptEngine, const QString &property) {
    if (property == "offsetPosition") {
        return vec3toScriptValue(scriptEngine, getOffsetPosition());
    }
    if (property == "offsetRotation") {
        return quatToScriptValue(scriptEngine, getOffsetRotation());
    }
    return QScriptValue();
}

void PanelAttachable::setProperties(const QScriptValue &properties) {
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
}
