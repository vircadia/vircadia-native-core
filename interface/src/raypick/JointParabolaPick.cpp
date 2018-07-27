//
//  Created by Sam Gondelman 7/2/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "JointParabolaPick.h"

#include "avatar/AvatarManager.h"

JointParabolaPick::JointParabolaPick(const std::string& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset,
    float speed, const glm::vec3& accelerationAxis, bool rotateAccelerationWithAvatar, bool scaleWithAvatar, PickFilter& filter, float maxDistance, bool enabled) :
    ParabolaPick(speed, accelerationAxis, rotateAccelerationWithAvatar, scaleWithAvatar, filter, maxDistance, enabled),
    _jointName(jointName),
    _posOffset(posOffset),
    _dirOffset(dirOffset)
{
}

PickParabola JointParabolaPick::getMathematicalPick() const {
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    int jointIndex = myAvatar->getJointIndex(QString::fromStdString(_jointName));
    bool useAvatarHead = _jointName == "Avatar";
    const int INVALID_JOINT = -1;
    if (jointIndex != INVALID_JOINT || useAvatarHead) {
        glm::vec3 jointPos = useAvatarHead ? myAvatar->getHeadPosition() : myAvatar->getAbsoluteJointTranslationInObjectFrame(jointIndex);
        glm::quat jointRot = useAvatarHead ? myAvatar->getHeadOrientation() : myAvatar->getAbsoluteJointRotationInObjectFrame(jointIndex);
        glm::vec3 avatarPos = myAvatar->getWorldPosition();
        glm::quat avatarRot = myAvatar->getWorldOrientation();

        glm::vec3 pos = useAvatarHead ? jointPos : avatarPos + (avatarRot * jointPos);
        glm::quat rot = useAvatarHead ? jointRot * glm::angleAxis(-PI / 2.0f, Vectors::RIGHT) : avatarRot * jointRot;

        // Apply offset
        pos = pos + (rot * (myAvatar->getSensorToWorldScale() * _posOffset));
        glm::vec3 dir = glm::normalize(rot * glm::normalize(_dirOffset));

        return PickParabola(pos, getSpeed() * dir, getAcceleration());
    }

    return PickParabola();
}
