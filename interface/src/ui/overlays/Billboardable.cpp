//
//  Billboardable.cpp
//  interface/src/ui/overlays
//
//  Created by Zander Otavka on 8/7/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Billboardable.h"

#include <Application.h>
#include <Transform.h>

void Billboardable::setProperties(const QScriptValue &properties) {
    QScriptValue isFacingAvatar = properties.property("isFacingAvatar");
    if (isFacingAvatar.isValid()) {
        setIsFacingAvatar(isFacingAvatar.toVariant().toBool());
    }
}

QScriptValue Billboardable::getProperty(QScriptEngine* scriptEngine, const QString &property) {
    if (property == "isFacingAvatar") {
        return isFacingAvatar();
    }
    return QScriptValue();
}

void Billboardable::pointTransformAtCamera(Transform& transform, glm::quat offsetRotation) {
    if (isFacingAvatar()) {
        glm::vec3 billboardPos = transform.getTranslation();
        glm::vec3 cameraPos = qApp->getCamera()->getPosition();
        glm::vec3 look = cameraPos - billboardPos;
        float elevation = -asinf(look.y / glm::length(look));
        float azimuth = atan2f(look.x, look.z);
        glm::quat rotation(glm::vec3(elevation, azimuth, 0));
        transform.setRotation(rotation);
        transform.postRotate(offsetRotation);
    }
}
