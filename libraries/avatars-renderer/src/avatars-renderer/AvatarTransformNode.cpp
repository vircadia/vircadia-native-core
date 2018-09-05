//
//  Created by Sabrina Shanman 9/5/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "AvatarTransformNode.h"

AvatarTransformNode::AvatarTransformNode(AvatarWeakPointer avatar, int jointIndex) :
    _avatar(avatar),
    _jointIndex(jointIndex)
{}

Transform AvatarTransformNode::getTransform() {
    auto avatar = _avatar.lock();
    if (!avatar) {
        return Transform();
    }

    bool success;
    Transform jointWorldTransform = avatar->getTransform(_jointIndex, success);
    if (!success) {
        jointWorldTransform = Transform();
    }

    jointWorldTransform.setScale(avatar->scaleForChildren());

    return jointWorldTransform;
}