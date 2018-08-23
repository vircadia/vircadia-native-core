//
//  Created by Sabrina Shanman 8/14/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MouseTransformNode.h"

#include "DependencyManager.h"
#include "PickManager.h"
#include "MouseRayPick.h"

const PickFilter MOUSE_TRANSFORM_NODE_PICK_FILTER(
    1 << PickFilter::PICK_ENTITIES |
    1 << PickFilter::PICK_AVATARS |
    1 << PickFilter::PICK_INCLUDE_NONCOLLIDABLE
);
const float MOUSE_TRANSFORM_NODE_MAX_DISTANCE = 1000.0f;

MouseTransformNode::MouseTransformNode() {
    _parentMouseRayPick = DependencyManager::get<PickManager>()->addPick(PickQuery::Ray,
        std::make_shared<MouseRayPick>(MOUSE_TRANSFORM_NODE_PICK_FILTER, MOUSE_TRANSFORM_NODE_MAX_DISTANCE, true));
}

MouseTransformNode::~MouseTransformNode() {
    if (DependencyManager::isSet<PickManager>()) {
        auto pickManager = DependencyManager::get<PickManager>();
        if (pickManager) {
            pickManager->removePick(_parentMouseRayPick);
        }
    }
}

Transform MouseTransformNode::getTransform() {
    Transform transform;
    std::shared_ptr<RayPickResult> rayPickResult = DependencyManager::get<PickManager>()->getPrevPickResultTyped<RayPickResult>(_parentMouseRayPick);
    if (rayPickResult) {
        transform.setTranslation(rayPickResult->intersection);
    }
    return transform;
}