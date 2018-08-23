//
//  Created by Sabrina Shanman 8/14/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_MouseTransformNode_h
#define hifi_MouseTransformNode_h

#include "TransformNode.h"

class MouseTransformNode : public TransformNode {
public:
    MouseTransformNode();
    ~MouseTransformNode();
    Transform getTransform() override;

protected:
    unsigned int _parentMouseRayPick = 0;
};

#endif // hifi_MouseTransformNode_h