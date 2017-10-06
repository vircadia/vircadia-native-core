//
//  JointRayPick.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "JointRayPick.h"

#include "DependencyManager.h"
#include "avatar/AvatarManager.h"

JointRayPick::JointRayPick(const std::string& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const RayPickFilter& filter, const float maxDistance, const bool enabled) :
    RayPick(filter, maxDistance, enabled),
    _jointName(jointName),
    _posOffset(posOffset),
    _dirOffset(dirOffset)
{
}

const PickRay JointRayPick::getPickRay(bool& valid) const {
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    int jointIndex = myAvatar->getJointIndex(QString::fromStdString(_jointName));
    bool useAvatarHead = _jointName == "Avatar";
    const int INVALID_JOINT = -1;
    if (jointIndex != INVALID_JOINT || useAvatarHead) {
        glm::vec3 jointPos = useAvatarHead ? myAvatar->getHeadPosition() : myAvatar->getAbsoluteJointTranslationInObjectFrame(jointIndex);
        glm::quat jointRot = useAvatarHead ? myAvatar->getHeadOrientation() : myAvatar->getAbsoluteJointRotationInObjectFrame(jointIndex);
        glm::vec3 avatarPos = myAvatar->getPosition();
        glm::quat avatarRot = myAvatar->getOrientation();

        glm::vec3 pos = useAvatarHead ? jointPos : avatarPos + (avatarRot * jointPos);
        glm::quat rot = useAvatarHead ? jointRot * glm::angleAxis(-PI / 2.0f, Vectors::RIGHT) : avatarRot * jointRot;

        // Apply offset
        pos = pos + (rot * (myAvatar->getSensorToWorldScale() * _posOffset));
        glm::vec3 dir = rot * glm::normalize(_dirOffset);

        valid = true;
        return PickRay(pos, dir);
    }

    valid = false;
    return PickRay();
}
