//
//  Created by Sam Gondelman 7/2/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "MouseParabolaPick.h"

#include "Application.h"
#include "display-plugins/CompositorHelper.h"

MouseParabolaPick::MouseParabolaPick(float speed, const glm::vec3& accelerationAxis, bool rotateAccelerationWithAvatar,
                                     bool scaleWithAvatar, const PickFilter& filter, float maxDistance, bool enabled) :
    ParabolaPick(speed, accelerationAxis, rotateAccelerationWithAvatar, scaleWithAvatar, filter, maxDistance, enabled)
{
}

PickParabola MouseParabolaPick::getMathematicalPick() const {
    QVariant position = qApp->getApplicationCompositor().getReticleInterface()->getPosition();
    if (position.isValid()) {
        QVariantMap posMap = position.toMap();
        PickRay pickRay = qApp->getCamera().computePickRay(posMap["x"].toFloat(), posMap["y"].toFloat());
        return PickParabola(pickRay.origin, getSpeed() * pickRay.direction, getAcceleration());
    }

    return PickParabola();
}
