//
//  MouseRayPick.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/19/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "MouseRayPick.h"

#include "Application.h"
#include "display-plugins/CompositorHelper.h"

MouseRayPick::MouseRayPick(const PickFilter& filter, float maxDistance, bool enabled) :
    RayPick(filter, maxDistance, enabled)
{
}

PickRay MouseRayPick::getMathematicalPick() const {
    QVariant position = qApp->getApplicationCompositor().getReticleInterface()->getPosition();
    if (position.isValid()) {
        QVariantMap posMap = position.toMap();
        return qApp->getCamera().computePickRay(posMap["x"].toFloat(), posMap["y"].toFloat());
    }

    return PickRay();
}
