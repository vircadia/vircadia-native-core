//
//  Created by Sabrina Shanman 2018/09/05
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "AvatarTransformNode.h"

template<>
glm::vec3 BaseNestableTransformNode<Avatar>::getActualScale(const std::shared_ptr<Avatar>& nestablePointer) const {
    return nestablePointer->scaleForChildren();
}
