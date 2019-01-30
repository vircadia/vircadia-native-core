//
//  Created by Sabrina Shanman 9/5/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OverlayTransformNode.h"

template<>
glm::vec3 BaseNestableTransformNode<Base3DOverlay>::getActualScale(const std::shared_ptr<Base3DOverlay>& nestablePointer) const {
    return nestablePointer->getBounds().getScale();
}