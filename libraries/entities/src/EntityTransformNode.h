//
//  Created by Sabrina Shanman 9/5/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_EntityTransformNode_h
#define hifi_EntityTransformNode_h

#include "TransformNode.h"

#include "EntityItem.h"

class EntityTransformNode : public TransformNode {
public:
    EntityTransformNode(EntityItemWeakPointer entity, int jointIndex);
    Transform getTransform() override;

protected:
    EntityItemWeakPointer _entity;
    int _jointIndex;
};

#endif // hifi_EntityTransformNode_h