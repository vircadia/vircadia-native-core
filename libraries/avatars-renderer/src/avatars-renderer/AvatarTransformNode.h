//
//  Created by Sabrina Shanman 9/5/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_AvatarTransformNode_h
#define hifi_AvatarTransformNode_h

#include "NestableTransformNode.h"

#include "Avatar.h"

class AvatarTransformNode : public TransformNode {
public:
    AvatarTransformNode(AvatarWeakPointer avatar, int jointIndex);
    Transform getTransform() override;

protected:
    AvatarWeakPointer _avatar;
    int _jointIndex;
};

#endif // hifi_AvatarTransformNode_h