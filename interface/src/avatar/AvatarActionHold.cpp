//
//  AvatarActionHold.cpp
//  interface/src/avatar/
//
//  Created by Seth Alves 2015-6-9
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "avatar/MyAvatar.h"
#include "avatar/AvatarManager.h"

#include "AvatarActionHold.h"

AvatarActionHold::AvatarActionHold(QUuid id, EntityItemPointer ownerEntity) :
    ObjectAction(id, ownerEntity) {
    #if WANT_DEBUG
    qDebug() << "AvatarActionHold::AvatarActionHold";
    #endif
}

AvatarActionHold::~AvatarActionHold() {
    #if WANT_DEBUG
    qDebug() << "AvatarActionHold::~AvatarActionHold";
    #endif
}

void AvatarActionHold::updateActionWorker(float deltaTimeStep) {
    // handle the linear part
    if (_linearOffsetSet) {
    }

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    glm::vec3 palmPosition = myAvatar->getRightPalmPosition();
    glm::vec3 position = getPosition();
    glm::vec3 positionalTarget = palmPosition + _linearOffset;
    glm::vec3 offset = positionalTarget - position;

    float offsetLength = glm::length(offset);
    float speed = offsetLength / _timeScale;

    if (offsetLength > IGNORE_POSITION_DELTA) {
        glm::vec3 newVelocity = glm::normalize(offset) * speed;
        setLinearVelocity(newVelocity);
    } else {
        setLinearVelocity(glm::vec3(0.0f));
    }


    // handle rotation
    if (_angularOffsetSet) {
        glm::quat bodyRotation = getRotation();
        // if qZero and qOne are too close to each other, we can get NaN for angle.
        auto alignmentDot = glm::dot(bodyRotation, _angularOffset);
        const float almostOne = 0.99999;
        if (glm::abs(alignmentDot) < almostOne) {
            glm::quat target = _angularOffset;
            if (alignmentDot < 0) {
                target = -target;
            }
            glm::quat qZeroInverse = glm::inverse(bodyRotation);
            glm::quat deltaQ = target * qZeroInverse;
            glm::vec3 axis = glm::axis(deltaQ);
            float angle = glm::angle(deltaQ);
            if (isNaN(angle)) {
                qDebug() << "AvatarActionHold::updateAction angle =" << angle
                         << "body-rotation =" << bodyRotation.x << bodyRotation.y << bodyRotation.z << bodyRotation.w
                         << "target-rotation ="
                         << target.x << target.y << target.z<< target.w;
            }
            assert(!isNaN(angle));
            glm::vec3 newAngularVelocity = (angle / _timeScale) * glm::normalize(axis);
            setAngularVelocity(newAngularVelocity);
        } else {
            setAngularVelocity(glm::vec3(0.0f));
        }
    }
}


bool AvatarActionHold::updateArguments(QVariantMap arguments) {
    // targets are required, spring-constants are optional
    bool ptOk = true;
    glm::vec3 linearOffset =
        EntityActionInterface::extractVec3Argument("spring action", arguments, "targetPosition", ptOk, false);

    bool rtOk = true;
    glm::quat angularOffset =
        EntityActionInterface::extractQuatArgument("spring action", arguments, "targetRotation", rtOk, false);

    lockForWrite();

    _linearOffsetSet = _angularOffsetSet = false;

    if (ptOk) {
        _linearOffset = linearOffset;
        _linearOffsetSet = true;
    }

    if (rtOk) {
        _angularOffset = angularOffset;
        _angularOffsetSet = true;
    }

    _active = true;
    unlock();
    return true;
}
