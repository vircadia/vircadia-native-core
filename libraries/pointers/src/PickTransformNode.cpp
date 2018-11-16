//
//  Created by Sabrina Shanman 8/22/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PickTransformNode.h"

#include "DependencyManager.h"
#include "PickManager.h"

PickTransformNode::PickTransformNode(unsigned int uid) :
    _uid(uid)
{
}

Transform PickTransformNode::getTransform() {
    auto pickManager = DependencyManager::get<PickManager>();
    if (!pickManager) {
        return Transform();
    }

    return pickManager->getResultTransform(_uid);
}