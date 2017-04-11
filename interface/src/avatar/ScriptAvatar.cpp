//
//  ScriptAvatar.cpp
//  interface/src/avatars
//
//  Created by Stephen Birarda on 4/10/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptAvatar.h"

ScriptAvatar::ScriptAvatar(AvatarSharedPointer avatarData) :
    ScriptAvatarData(avatarData)
{

}

std::shared_ptr<Avatar> ScriptAvatar::lockAvatar() const {
    if (auto lockedAvatarData = _avatarData.lock()) {
        return std::dynamic_pointer_cast<Avatar>(lockedAvatarData);
    } else {
        return std::shared_ptr<Avatar>();
    }
}

glm::quat ScriptAvatar::getDefaultJointRotation(int index) const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getDefaultJointRotation(index);
    } else {
        return glm::quat();
    }
}

glm::vec3 ScriptAvatar::getDefaultJointTranslation(int index) const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getDefaultJointTranslation(index);
    } else {
        return glm::vec3();
    }
}

glm::vec3 ScriptAvatar::getSkeletonOffset() const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getSkeletonOffset();
    } else {
        return glm::vec3();
    }
}

glm::vec3 ScriptAvatar::getJointPosition(int index) const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getJointPosition(index);
    } else {
        return glm::vec3();
    }
}

glm::vec3 ScriptAvatar::getJointPosition(const QString& name) const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getJointPosition(name);
    } else {
        return glm::vec3();
    }
}

glm::vec3 ScriptAvatar::getNeckPosition() const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getNeckPosition();
    } else {
        return glm::vec3();
    }
}

glm::vec3 ScriptAvatar::getAcceleration() const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getAcceleration();
    } else {
        return glm::vec3();
    }
}

QUuid ScriptAvatar::getParentID() const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getParentID();
    } else {
        return QUuid();
    }
}

quint16 ScriptAvatar::getParentJointIndex() const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getParentJointIndex();
    } else {
        return INVALID_JOINT_INDEX;
    }
}

QVariantList ScriptAvatar::getSkeleton() const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getSkeleton();
    } else {
        return QVariantList();
    }
}

float ScriptAvatar::getSimulationRate(const QString& rateName) const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getSimulationRate(rateName);
    } else {
        return 0.0f;;
    }
}

glm::vec3 ScriptAvatar::getLeftPalmPosition() const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getLeftPalmPosition();
    } else {
        return glm::vec3();
    }
}

glm::quat ScriptAvatar::getLeftPalmRotation() const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getLeftPalmRotation();
    } else {
        return glm::quat();
    }
}

glm::vec3 ScriptAvatar::getRightPalmPosition() const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getRightPalmPosition();
    } else {
        return glm::vec3();
    }
}

glm::quat ScriptAvatar::getRightPalmRotation() const {
    if (auto lockedAvatar = lockAvatar()) {
        return lockedAvatar->getRightPalmRotation();
    } else {
        return glm::quat();
    }
}
