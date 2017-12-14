//
//  PanelAttachable.cpp
//  interface/src/ui/overlays
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
#if OVERLAY_PANELS
    if (getParentPanel()) {
        return getParentPanel()->getVisible() && getParentPanel()->getParentVisible();
    } else {
        return true;
    }
#else
    return true;
#endif
}

// JSDoc for copying to @typedefs of overlay types that inherit PanelAttachable.
// No JSDoc because these properties are not actually used.
QVariant PanelAttachable::getProperty(const QString& property) {
    if (property == "offsetPosition") {
        return vec3toVariant(getOffsetPosition());
    }
    if (property == "offsetRotation") {
        return quatToVariant(getOffsetRotation());
    }
    if (property == "offsetScale") {
        return vec3toVariant(getOffsetScale());
    }
    return QVariant();
}

void PanelAttachable::setProperties(const QVariantMap& properties) {
    auto offsetPosition = properties["offsetPosition"];
    bool valid;
    if (offsetPosition.isValid()) {
        glm::vec3 newPosition = vec3FromVariant(offsetPosition, valid);
        if (valid) {
            setOffsetPosition(newPosition);
        }
    }

    auto offsetRotation = properties["offsetRotation"];
    if (offsetRotation.isValid()) {
        setOffsetRotation(quatFromVariant(offsetRotation));
    }

    auto offsetScale = properties["offsetScale"];
    if (offsetScale.isValid()) {
        setOffsetScale(vec3FromVariant(offsetScale));
    }
}

bool PanelAttachable::applyTransformTo(Transform& transform, bool force) {
    if (force || usecTimestampNow() > _transformExpiry) {
        const quint64 TRANSFORM_UPDATE_PERIOD = 100000; // frequency is 10 Hz
        _transformExpiry = usecTimestampNow() + TRANSFORM_UPDATE_PERIOD;
#if OVERLAY_PANELS
        if (getParentPanel()) {
            getParentPanel()->applyTransformTo(transform, true);
            transform.postTranslate(getOffsetPosition());
            transform.postRotate(getOffsetRotation());
            transform.postScale(getOffsetScale());
            return true;
        }
#endif
    }
    return false;
}
