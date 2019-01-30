//
//  Created by Sabrina Shanman 8/14/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_MyAvatarHeadTransformNode_h
#define hifi_MyAvatarHeadTransformNode_h

#include "TransformNode.h"

class MyAvatarHeadTransformNode : public TransformNode {
public:
    MyAvatarHeadTransformNode() { }
    Transform getTransform() override;
};

#endif // hifi_MyAvatarHeadTransformNode_h