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
    _hand("right")
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
    bool gotLock = false;
    glm::quat rotation;
    glm::vec3 position;
    glm::vec3 offset;

    gotLock = withTryReadLock([&]{
        auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        glm::vec3 palmPosition;
        glm::quat palmRotation;
        if (_hand == "right") {
            palmPosition = myAvatar->getRightPalmPosition();
            palmRotation = myAvatar->getRightPalmRotation();
        } else {
            palmPosition = myAvatar->getLeftPalmPosition();
            palmRotation = myAvatar->getLeftPalmRotation();
        }

        rotation = palmRotation * _relativeRotation;
        offset = rotation * _relativePosition;
        position = palmPosition + offset;
    });

    if (gotLock) {
        gotLock = withTryWriteLock([&]{
            _positionalTarget = position;
            _rotationalTarget = rotation;
            _positionalTargetSet = true;
            _rotationalTargetSet = true;
        });
    }
    if (gotLock) {
        ObjectActionSpring::updateActionWorker(deltaTimeStep);
    }
}


bool AvatarActionHold::updateArguments(QVariantMap arguments) {
    glm::vec3 relativePosition;
    glm::quat relativeRotation;
    float timeScale;
    QString hand;
    QUuid holderID;
    bool needUpdate = false;

    bool somethingChanged = ObjectAction::updateArguments(arguments);
    withReadLock([&]{
        bool ok = true;
        relativePosition = EntityActionInterface::extractVec3Argument("hold", arguments, "relativePosition", ok, false);
        if (!ok) {
            relativePosition = _relativePosition;
        }

        ok = true;
        relativeRotation = EntityActionInterface::extractQuatArgument("hold", arguments, "relativeRotation", ok, false);
        if (!ok) {
            relativeRotation = _relativeRotation;
        }

        ok = true;
        timeScale = EntityActionInterface::extractFloatArgument("hold", arguments, "timeScale", ok, false);
        if (!ok) {
            timeScale = _linearTimeScale;
        }

        ok = true;
        hand = EntityActionInterface::extractStringArgument("hold", arguments, "hand", ok, false);
        if (!ok || !(hand == "left" || hand == "right")) {
            hand = _hand;
        }

        ok = true;
        auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        holderID = myAvatar->getSessionUUID();

        if (somethingChanged ||
            relativePosition != _relativePosition ||
            relativeRotation != _relativeRotation ||
            timeScale != _linearTimeScale ||
            hand != _hand) {
            needUpdate = true;
        }
    });

    if (needUpdate) {
        withWriteLock([&] {
            _relativePosition = relativePosition;
            _relativeRotation = relativeRotation;
            const float MIN_TIMESCALE = 0.1f;
            _linearTimeScale = glm::max(MIN_TIMESCALE, timeScale);
            _angularTimeScale = _linearTimeScale;
            _hand = hand;
            _active = true;

            auto ownerEntity = _ownerEntity.lock();
            if (ownerEntity) {
                ownerEntity->setActionDataDirty(true);
            }
        });
        activateBody();
    }

    return true;
}

QVariantMap AvatarActionHold::getArguments() {
    QVariantMap arguments = ObjectAction::getArguments();
    withReadLock([&]{
        arguments["relativePosition"] = glmToQMap(_relativePosition);
        arguments["relativeRotation"] = glmToQMap(_relativeRotation);
        arguments["timeScale"] = _linearTimeScale;
        arguments["hand"] = _hand;
    });
    return arguments;
}

QByteArray AvatarActionHold::serialize() const {
    return ObjectActionSpring::serialize();
}

void AvatarActionHold::deserialize(QByteArray serializedArguments) {
    assert(false);
}
