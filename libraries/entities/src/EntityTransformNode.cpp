//
//  Created by Sabrina Shanman 9/5/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "EntityTransformNode.h"

EntityTransformNode::EntityTransformNode(EntityItemWeakPointer entity, int jointIndex) :
    _entity(entity),
    _jointIndex(jointIndex)
{}

Transform EntityTransformNode::getTransform() {
    auto entity = _entity.lock();
    if (!entity) {
        return Transform();
    }

    bool success;
    Transform jointWorldTransform = entity->getTransform(_jointIndex, success);
    if (!success) {
        return Transform();
    }

    jointWorldTransform.setScale(entity->getScaledDimensions());

    return jointWorldTransform;
}