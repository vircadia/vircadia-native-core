//
//  SkeletonModel.cpp
//  interface
//
//  Created by Andrzej Kapolka on 10/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "Avatar.h"
#include "SkeletonModel.h"

SkeletonModel::SkeletonModel(Avatar* owningAvatar) :
    _owningAvatar(owningAvatar)
{
}

void SkeletonModel::simulate(float deltaTime) {
    if (!isActive()) {
        return;
    }
    
    setTranslation(_owningAvatar->getPosition());
    setRotation(_owningAvatar->getOrientation());
    const float MODEL_SCALE = 0.0006f;
    setScale(glm::vec3(-1.0f, 1.0f, -1.0f) * _owningAvatar->getScale() * MODEL_SCALE);
    
    Model::simulate(deltaTime);
}
