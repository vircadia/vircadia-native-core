//
//  Created by Sabrina Shanman 9/5/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OverlayTransformNode.h"

OverlayTransformNode::OverlayTransformNode(std::weak_ptr<Base3DOverlay> overlay, int jointIndex) :
    _overlay(overlay),
    _jointIndex(jointIndex)
{}

Transform OverlayTransformNode::getTransform() {
    auto overlay = _overlay.lock();
    if (!overlay) {
        return Transform();
    }
    
    bool success;
    Transform jointWorldTransform = overlay->getTransform(_jointIndex, success);
    if (!success) {
        return Transform();
    }

    jointWorldTransform.setScale(overlay->getBounds().getScale());

    return jointWorldTransform;
}