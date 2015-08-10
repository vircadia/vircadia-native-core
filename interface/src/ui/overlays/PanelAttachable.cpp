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

QScriptValue PanelAttachable::getProperty(QScriptEngine* scriptEngine, const QString &property) {
    if (property == "offsetPosition") {
        return vec3toScriptValue(scriptEngine, getOffsetPosition());
    }
    if (property == "offsetRotation") {
        return quatToScriptValue(scriptEngine, getOffsetRotation());
    }
    if (property == "offsetScale") {
        return vec3toScriptValue(scriptEngine, getOffsetScale());
    }
    return QScriptValue();
}

void PanelAttachable::setProperties(const QScriptValue &properties) {
    QScriptValue offsetPosition = properties.property("offsetPosition");
    if (offsetPosition.isValid() &&
        offsetPosition.property("x").isValid() &&
        offsetPosition.property("y").isValid() &&
        offsetPosition.property("z").isValid()) {
        glm::vec3 newPosition;
        vec3FromScriptValue(offsetPosition, newPosition);
        setOffsetPosition(newPosition);
    }

    QScriptValue offsetRotation = properties.property("offsetRotation");
    if (offsetRotation.isValid() &&
        offsetRotation.property("x").isValid() &&
        offsetRotation.property("y").isValid() &&
        offsetRotation.property("z").isValid() &&
        offsetRotation.property("w").isValid()) {
        glm::quat newRotation;
        quatFromScriptValue(offsetRotation, newRotation);
        setOffsetRotation(newRotation);
    }

    QScriptValue offsetScale = properties.property("offsetScale");
    if (offsetScale.isValid()) {
        if (offsetScale.property("x").isValid() &&
            offsetScale.property("y").isValid() &&
            offsetScale.property("z").isValid()) {
            glm::vec3 newScale;
            vec3FromScriptValue(offsetScale, newScale);
            setOffsetScale(newScale);
        } else {
            setOffsetScale(offsetScale.toVariant().toFloat());
        }
    }
}

void PanelAttachable::applyTransformTo(Transform& transform, bool force) {
    if (force || usecTimestampNow() > _transformExpiry) {
        const quint64 TRANSFORM_UPDATE_PERIOD = 100000; // frequency is 10 Hz
        _transformExpiry = usecTimestampNow() + TRANSFORM_UPDATE_PERIOD;
        if (getParentPanel()) {
            getParentPanel()->applyTransformTo(transform, true);
            transform.postTranslate(getOffsetPosition());
            transform.postRotate(getOffsetRotation());
            transform.postScale(getOffsetScale());
        }
    }
}
