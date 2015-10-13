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
    _holderID(QUuid())
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
    QSharedPointer<AvatarManager> avatarManager = DependencyManager::get<AvatarManager>();
    AvatarSharedPointer holdingAvatarData = avatarManager->getAvatarBySessionID(_holderID);
    std::shared_ptr<Avatar> holdingAvatar = std::static_pointer_cast<Avatar>(holdingAvatarData);

    if (holdingAvatar) {
        glm::quat rotation;
        glm::vec3 position;
        glm::vec3 offset;
        bool gotLock = withTryReadLock([&]{
                glm::vec3 palmPosition;
                glm::quat palmRotation;
                if (_hand == "right") {
                    palmPosition = holdingAvatar->getRightPalmPosition();
                    palmRotation = holdingAvatar->getRightPalmRotation();
                } else {
                    palmPosition = holdingAvatar->getLeftPalmPosition();
                    palmRotation = holdingAvatar->getLeftPalmRotation();
                }

                rotation = palmRotation * _relativeRotation;
                offset = rotation * _relativePosition;
                position = palmPosition + offset;
            });

        if (gotLock) {
            gotLock = withTryWriteLock([&]{
                    if (_positionalTarget != position || _rotationalTarget != rotation) {
                        auto ownerEntity = _ownerEntity.lock();
                        _positionalTarget = position;
                        _rotationalTarget = rotation;
                    }
                });
        }

        if (gotLock) {
            ObjectActionSpring::updateActionWorker(deltaTimeStep);
        }
    }
}


bool AvatarActionHold::updateArguments(QVariantMap arguments) {
    if (!ObjectAction::updateArguments(arguments)) {
        return false;
    }
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

    ok = true;
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    auto holderID = myAvatar->getSessionUUID();
    QString holderIDString =
        EntityActionInterface::extractStringArgument("hold", arguments, "hand", ok, false);
    if (ok) {
        holderID = QUuid(holderIDString);
    }

    if (relativePosition != _relativePosition
            || relativeRotation != _relativeRotation
            || timeScale != _linearTimeScale
            || hand != _hand
            || holderID != _holderID) {
        withWriteLock([&] {
            _relativePosition = relativePosition;
            _relativeRotation = relativeRotation;
            const float MIN_TIMESCALE = 0.1f;
            _linearTimeScale = glm::max(MIN_TIMESCALE, timeScale);
            _angularTimeScale = _linearTimeScale;
            _hand = hand;
            _holderID = holderID;

            _active = true;
            activateBody();

            auto ownerEntity = _ownerEntity.lock();
            if (ownerEntity) {
                ownerEntity->setActionDataDirty(true);
            }
        });
    }

    return true;
}

QVariantMap AvatarActionHold::getArguments() {
    QVariantMap arguments = ObjectAction::getArguments();
    withReadLock([&]{
        arguments["holderID"] = _holderID;
        arguments["relativePosition"] = glmToQMap(_relativePosition);
        arguments["relativeRotation"] = glmToQMap(_relativeRotation);
        arguments["timeScale"] = _linearTimeScale;
        arguments["hand"] = _hand;
    });
    return arguments;
}

QByteArray AvatarActionHold::serialize() const {
    QByteArray serializedActionArguments;
    QDataStream dataStream(&serializedActionArguments, QIODevice::WriteOnly);

    dataStream << ACTION_TYPE_SPRING;
    dataStream << getID();
    dataStream << AvatarActionHold::holdVersion;

    dataStream << _holderID;
    dataStream << _relativePosition;
    dataStream << _relativeRotation;
    dataStream << _linearTimeScale;
    dataStream << _hand;

    dataStream << _expires;
    dataStream << _tag;

    return serializedActionArguments;
}

void AvatarActionHold::deserialize(QByteArray serializedArguments) {
    QDataStream dataStream(serializedArguments);

    EntityActionType type;
    dataStream >> type;
    assert(type == getType());

    QUuid id;
    dataStream >> id;
    assert(id == getID());

    uint16_t serializationVersion;
    dataStream >> serializationVersion;
    if (serializationVersion != AvatarActionHold::holdVersion) {
        return;
    }

    dataStream >> _holderID;
    dataStream >> _relativePosition;
    dataStream >> _relativeRotation;
    dataStream >> _linearTimeScale;
    _angularTimeScale = _linearTimeScale;
    dataStream >> _hand;

    dataStream >> _expires;
    dataStream >> _tag;

    _active = true;
}
