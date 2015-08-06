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

PanelAttachable::PanelAttachable() :
    _attachedPanel(nullptr),
    _offsetPosition(0, 0, 0),
    _offsetRotation(1, 0, 0, 0)
{
}

PanelAttachable::PanelAttachable(const PanelAttachable* panelAttachable) :
    _attachedPanel(panelAttachable->_attachedPanel),
    _offsetPosition(panelAttachable->_offsetPosition),
    _offsetRotation(panelAttachable->_offsetRotation)
{
}

bool PanelAttachable::getParentVisible() const {
    if (getAttachedPanel()) {
        return getAttachedPanel()->getVisible() && getAttachedPanel()->getParentVisible();
    } else {
        return true;
    }
}

void PanelAttachable::setTransforms(Transform& transform) {
    if (getAttachedPanel()) {
        transform.setTranslation(getAttachedPanel()->getComputedPosition());
        transform.setRotation(getAttachedPanel()->getComputedRotation());
        transform.postTranslate(getAttachedPanel()->getOffsetPosition());
        transform.postRotate(getAttachedPanel()->getFacingRotation());
        transform.postTranslate(getOffsetPosition());
        transform.postRotate(getFacingRotation());
    }
}

QScriptValue PanelAttachable::getProperty(QScriptEngine* scriptEngine, const QString &property) {
    if (property == "offsetPosition") {
        return vec3toScriptValue(scriptEngine, getOffsetPosition());
    }
    if (property == "offsetRotation") {
        return quatToScriptValue(scriptEngine, getFacingRotation());
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
            setFacingRotation(newRotation);
        }
    }
}
