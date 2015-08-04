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
#include "Application.h"

Billboard3DOverlay::Billboard3DOverlay() :
    _isFacingAvatar(false)
{
}

Billboard3DOverlay::Billboard3DOverlay(const Billboard3DOverlay* billboard3DOverlay) :
    Planar3DOverlay(billboard3DOverlay),
    PanelAttachable(billboard3DOverlay),
    _isFacingAvatar(billboard3DOverlay->_isFacingAvatar)
{
}

void Billboard3DOverlay::setProperties(const QScriptValue &properties) {
    Planar3DOverlay::setProperties(properties);
    PanelAttachable::setProperties(properties);

    QScriptValue isFacingAvatarValue = properties.property("isFacingAvatar");
    if (isFacingAvatarValue.isValid()) {
        _isFacingAvatar = isFacingAvatarValue.toVariant().toBool();
    }
}

QScriptValue Billboard3DOverlay::getProperty(const QString &property) {
    if (property == "isFacingAvatar") {
        return _isFacingAvatar;
    }

    QScriptValue value = PanelAttachable::getProperty(_scriptEngine, property);
    if (value.isValid()) {
        return value;
    }
    return Planar3DOverlay::getProperty(property);
}

void Billboard3DOverlay::setTransforms(Transform& transform) {
    PanelAttachable::setTransforms(transform);
    if (_isFacingAvatar) {
        glm::quat rotation = Application::getInstance()->getCamera()->getOrientation();
        transform.setRotation(rotation);
    }
}
