//
//  Created by Sabrina Shanman 2018/08/14
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MouseTransformNode.h"

#include "Application.h"
#include "display-plugins/CompositorHelper.h"
#include "RayPick.h"

Transform MouseTransformNode::getTransform() {
    QVariant position = qApp->getApplicationCompositor().getReticleInterface()->getPosition();
    if (position.isValid()) {
        Transform transform;
        QVariantMap posMap = position.toMap();
        PickRay pickRay = qApp->getCamera().computePickRay(posMap["x"].toFloat(), posMap["y"].toFloat());
        transform.setTranslation(pickRay.origin);
        transform.setRotation(rotationBetween(Vectors::UP, pickRay.direction));
        return transform;
    }

    return Transform();
}

QVariantMap MouseTransformNode::toVariantMap() const {
    QVariantMap map;
    map["joint"] = "Mouse";
    return map;
}
