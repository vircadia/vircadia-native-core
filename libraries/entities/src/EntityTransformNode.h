//
//  Created by Sabrina Shanman 9/5/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_EntityTransformNode_h
#define hifi_EntityTransformNode_h

#include "NestableTransformNode.h"

#include "EntityItem.h"

class EntityTransformNode : public BaseNestableTransformNode<EntityItem> {
public:
    EntityTransformNode(std::weak_ptr<EntityItem> spatiallyNestable, int jointIndex) : BaseNestableTransformNode(spatiallyNestable, jointIndex) {};
};

#endif // hifi_EntityTransformNode_h