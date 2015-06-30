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

#include "QVariantGLM.h"
#include "avatar/MyAvatar.h"
#include "avatar/AvatarManager.h"

#include "AvatarActionHold.h"

const uint16_t AvatarActionHold::holdVersion = 1;

AvatarActionHold::AvatarActionHold(EntityActionType type, QUuid id, EntityItemPointer ownerEntity) :
    ObjectActionSpring(type, id, ownerEntity),
    _relativePosition(glm::vec3(0.0f)),
    _relativeRotation(glm::quat()),
    _hand("right"),
    _mine(false)
{
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
    if (!_mine) {
        // if a local script isn't updating this, then we are just getting spring-action data over the wire.
        // let the super-class handle it.
        ObjectActionSpring::updateActionWorker(deltaTimeStep);
        return;
    }

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    if (!tryLockForRead()) {
        // don't risk hanging the thread running the physics simulation
        return;
    }

    glm::vec3 palmPosition;
    if (_hand == "right") {
        palmPosition = myAvatar->getRightPalmPosition();
    } else {
        palmPosition = myAvatar->getLeftPalmPosition();
    }

    auto rotation = myAvatar->getWorldAlignedOrientation();
    auto offset = rotation * _relativePosition;
    auto position = palmPosition + offset;
    rotation *= _relativeRotation;
    unlock();

    if (!tryLockForWrite()) {
        return;
    }

    // check for NaNs
    if (position.x != position.x ||
        position.y != position.y ||
        position.z != position.z) {
        qDebug() << "AvatarActionHold::updateActionWorker -- target position includes NaN";
        return;
    }
    if (rotation.x != rotation.x ||
        rotation.y != rotation.y ||
        rotation.z != rotation.z ||
        rotation.w != rotation.w) {
        qDebug() << "AvatarActionHold::updateActionWorker -- target rotation includes NaN";
        return;
    }

    _positionalTarget = position;
    _rotationalTarget = rotation;
    unlock();

    ObjectActionSpring::updateActionWorker(deltaTimeStep);
}


bool AvatarActionHold::updateArguments(QVariantMap arguments) {
    bool rPOk = true;
    glm::vec3 relativePosition =
        EntityActionInterface::extractVec3Argument("hold", arguments, "relativePosition", rPOk, false);
    bool rROk = true;
    glm::quat relativeRotation =
        EntityActionInterface::extractQuatArgument("hold", arguments, "relativeRotation", rROk, false);
    bool tSOk = true;
    float timeScale =
        EntityActionInterface::extractFloatArgument("hold", arguments, "timeScale", tSOk, false);
    bool hOk = true;
    QString hand =
        EntityActionInterface::extractStringArgument("hold", arguments, "hand", hOk, false);

    lockForWrite();
    if (rPOk) {
        _relativePosition = relativePosition;
    } else {
        _relativePosition = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    if (rROk) {
        _relativeRotation = relativeRotation;
    } else {
        _relativeRotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
    }

    if (tSOk) {
        _linearTimeScale = timeScale;
        _angularTimeScale = timeScale;
    } else {
        _linearTimeScale = 0.2f;
        _angularTimeScale = 0.2f;
    }

    if (hOk) {
        hand = hand.toLower();
        if (hand == "left") {
            _hand = "left";
        } else if (hand == "right") {
            _hand = "right";
        } else {
            qDebug() << "hold action -- invalid hand argument:" << hand;
            _hand = "right";
        }
    } else {
        _hand = "right";
    }

    _mine = true;
    _positionalTargetSet = true;
    _rotationalTargetSet = true;
    _active = true;
    unlock();
    return true;
}


QVariantMap AvatarActionHold::getArguments() {
    QVariantMap arguments;
    lockForRead();
    if (_mine) {
        arguments["relativePosition"] = glmToQMap(_relativePosition);
        arguments["relativeRotation"] = glmToQMap(_relativeRotation);
        arguments["timeScale"] = _linearTimeScale;
        arguments["hand"] = _hand;
    } else {
        unlock();
        return ObjectActionSpring::getArguments();
    }
    unlock();
    return arguments;
}


void AvatarActionHold::deserialize(QByteArray serializedArguments) {
    if (_mine) {
        return;
    }
    ObjectActionSpring::deserialize(serializedArguments);
}
