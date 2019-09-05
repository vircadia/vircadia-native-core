//
//  Created by Sabrina Shanman 2018/08/14
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NestableTransformNode.h"

template<>
glm::vec3 BaseNestableTransformNode<SpatiallyNestable>::getActualScale(const std::shared_ptr<SpatiallyNestable>& nestablePointer) const {
    return nestablePointer->getAbsoluteJointScaleInObjectFrame(_jointIndex);
}
