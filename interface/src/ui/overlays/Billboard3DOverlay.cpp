//
//  Billboard3DOverlay.cpp
//  hifi/interface/src/ui/overlays
//
//  Created by Zander Otavka on 8/4/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Billboard3DOverlay.h"

Billboard3DOverlay::Billboard3DOverlay(const Billboard3DOverlay* billboard3DOverlay) :
    Planar3DOverlay(billboard3DOverlay),
    PanelAttachable(*billboard3DOverlay),
    Billboardable(*billboard3DOverlay)
{
}

void Billboard3DOverlay::setProperties(const QVariantMap& properties) {
    Planar3DOverlay::setProperties(properties);
    PanelAttachable::setProperties(properties);
    Billboardable::setProperties(properties);
}

QVariant Billboard3DOverlay::getProperty(const QString &property) {
    QVariant value;
    value = Billboardable::getProperty(property);
    if (value.isValid()) {
        return value;
    }
    value = PanelAttachable::getProperty(property);
    if (value.isValid()) {
        return value;
    }
    return Planar3DOverlay::getProperty(property);
}

bool Billboard3DOverlay::applyTransformTo(Transform& transform, bool force) {
    bool transformChanged = false;
    if (force || usecTimestampNow() > _transformExpiry) {
        transformChanged = PanelAttachable::applyTransformTo(transform, true);
        transformChanged |= pointTransformAtCamera(transform, getOffsetRotation());
    }
    return transformChanged;
}
