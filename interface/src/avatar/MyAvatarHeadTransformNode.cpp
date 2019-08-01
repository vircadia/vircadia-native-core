//
//  Created by Sabrina Shanman 2018/08/14
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MyAvatarHeadTransformNode.h"

#include "DependencyManager.h"
#include "AvatarManager.h"
#include "MyAvatar.h"

Transform MyAvatarHeadTransformNode::getTransform() {
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    glm::vec3 pos = myAvatar->getHeadPosition();
    glm::vec3 scale = glm::vec3(myAvatar->scaleForChildren());
    glm::quat headOri = myAvatar->getHeadOrientation();
    glm::quat ori = headOri * glm::angleAxis(-PI / 2.0f, Vectors::RIGHT);

    return Transform(ori, scale, pos);
}

QVariantMap MyAvatarHeadTransformNode::toVariantMap() const {
    QVariantMap map;
    map["joint"] = "Avatar";
    return map;
}
