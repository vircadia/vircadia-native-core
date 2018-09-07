//
//  Created by Sabrina Shanman 8/14/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_NestableTransformNode_h
#define hifi_NestableTransformNode_h

#include "TransformNode.h"

#include "SpatiallyNestable.h"

class NestableTransformNode : public TransformNode {
public:
    NestableTransformNode(SpatiallyNestableWeakPointer spatiallyNestable, int jointIndex);
    Transform getTransform() override;

protected:
    SpatiallyNestableWeakPointer _spatiallyNestable;
    int _jointIndex;
};

#endif // hifi_NestableTransformNode_h