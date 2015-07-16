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

AvatarActionHold::AvatarActionHold(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectActionSpring(id, ownerEntity),
    _relativePosition(glm::vec3(0.0f)),
    _relativeRotation(glm::quat()),
    _hand("right"),
    _mine(false)
{
    _type = ACTION_TYPE_HOLD;
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
    glm::quat palmRotation;
    if (_hand == "right") {
        palmPosition = myAvatar->getRightPalmPosition();
        palmRotation = myAvatar->getRightPalmRotation();
    } else {
        palmPosition = myAvatar->getLeftPalmPosition();
        palmRotation = myAvatar->getLeftPalmRotation();
    }

    auto rotation = palmRotation * _relativeRotation;
    auto offset = rotation * _relativePosition;
    auto position = palmPosition + offset;
    unlock();

    if (!tryLockForWrite()) {
        return;
    }

    if (_positionalTarget != position || _rotationalTarget != rotation) {
        auto ownerEntity = _ownerEntity.lock();
        if (ownerEntity) {
            ownerEntity->setActionDataDirty(true);
        }
        _positionalTarget = position;
        _rotationalTarget = rotation;
    }
    unlock();

    ObjectActionSpring::updateActionWorker(deltaTimeStep);
}


bool AvatarActionHold::updateArguments(QVariantMap arguments) {
    bool ok = true;
    glm::vec3 relativePosition =
        EntityActionInterface::extractVec3Argument("hold", arguments, "relativePosition", ok, false);
    if (!ok) {
        relativePosition = _relativePosition;
    }

    ok = true;
    glm::quat relativeRotation =
        EntityActionInterface::extractQuatArgument("hold", arguments, "relativeRotation", ok, false);
    if (!ok) {
        relativeRotation = _relativeRotation;
    }
    
    ok = true;
    float timeScale =
        EntityActionInterface::extractFloatArgument("hold", arguments, "timeScale", ok, false);
    if (!ok) {
        timeScale = _linearTimeScale;
    }

    ok = true;
    QString hand =
        EntityActionInterface::extractStringArgument("hold", arguments, "hand", ok, false);
    if (!ok || !(hand == "left" || hand == "right")) {
        hand = _hand;
    }

    if (relativePosition != _relativePosition
            || relativeRotation != _relativeRotation
            || timeScale != _linearTimeScale
            || hand != _hand) {
        lockForWrite();
        _relativePosition = relativePosition;
        _relativeRotation = relativeRotation;
        const float MIN_TIMESCALE = 0.1f;
        _linearTimeScale = glm::min(MIN_TIMESCALE, timeScale);
        _angularTimeScale = _linearTimeScale;
        _hand = hand;

        _mine = true;
        _active = true;
        activateBody();
        unlock();
    }
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
    if (!_mine) {
        ObjectActionSpring::deserialize(serializedArguments);
    }
}
