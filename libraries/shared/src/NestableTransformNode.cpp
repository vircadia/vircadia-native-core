//
//  Created by Sabrina Shanman 8/14/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NestableTransformNode.h"

NestableTransformNode::NestableTransformNode(SpatiallyNestableWeakPointer spatiallyNestable, int jointIndex) :
    _spatiallyNestable(spatiallyNestable),
    _jointIndex(jointIndex)
{
}

Transform NestableTransformNode::getTransform() {
    auto nestable = _spatiallyNestable.lock();
    if (!nestable) {
        return Transform();
    }

    bool success;
    Transform jointWorldTransform = nestable->getTransform(_jointIndex, success);

    if (success) {
        return jointWorldTransform;
    } else {
        return Transform();
    }
}