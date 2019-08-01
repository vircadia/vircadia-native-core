//
//  Created by Sabrina Shanman 2018/09/05
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_AvatarTransformNode_h
#define hifi_AvatarTransformNode_h

#include "NestableTransformNode.h"

#include "Avatar.h"

class AvatarTransformNode : public BaseNestableTransformNode<Avatar> {
public:
    AvatarTransformNode(std::weak_ptr<Avatar> spatiallyNestable, int jointIndex) : BaseNestableTransformNode(spatiallyNestable, jointIndex) {};
};

#endif // hifi_AvatarTransformNode_h
