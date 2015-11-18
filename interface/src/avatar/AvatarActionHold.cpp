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
    bool gotLock = false;
    glm::quat rotation;
    glm::vec3 position;
    std::shared_ptr<Avatar> holdingAvatar = nullptr;

    gotLock = withTryReadLock([&]{
        QSharedPointer<AvatarManager> avatarManager = DependencyManager::get<AvatarManager>();
        AvatarSharedPointer holdingAvatarData = avatarManager->getAvatarBySessionID(_holderID);
        holdingAvatar = std::static_pointer_cast<Avatar>(holdingAvatarData);

        if (holdingAvatar) {
            glm::vec3 offset;
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
        }
    });

    if (holdingAvatar) {
        if (gotLock) {
            gotLock = withTryWriteLock([&]{
                _positionalTarget = position;
                _rotationalTarget = rotation;
                _positionalTargetSet = true;
                _rotationalTargetSet = true;
                _active = true;
            });
            if (gotLock) {
                if (_kinematic) {
                    doKinematicUpdate(deltaTimeStep);
                } else {
                    activateBody();
                    ObjectActionSpring::updateActionWorker(deltaTimeStep);
                }
            }
        }
    }
}

void AvatarActionHold::doKinematicUpdate(float deltaTimeStep) {
    auto ownerEntity = _ownerEntity.lock();
    if (!ownerEntity) {
        qDebug() << "AvatarActionHold::doKinematicUpdate -- no owning entity";
        return;
    }
    void* physicsInfo = ownerEntity->getPhysicsInfo();
    if (!physicsInfo) {
        qDebug() << "AvatarActionHold::doKinematicUpdate -- no owning physics info";
        return;
    }
    ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
    btRigidBody* rigidBody = motionState ? motionState->getRigidBody() : nullptr;
    if (!rigidBody) {
        qDebug() << "AvatarActionHold::doKinematicUpdate -- no rigidBody";
        return;
    }

    withWriteLock([&]{
        if (_kinematicSetVelocity) {
            if (_previousSet) {
                // smooth velocity over 2 frames
                glm::vec3 positionalDelta = _positionalTarget - _previousPositionalTarget;
                glm::vec3 positionalVelocity = (positionalDelta + _previousPositionalDelta) / (deltaTimeStep + _previousDeltaTimeStep);
                rigidBody->setLinearVelocity(glmToBullet(positionalVelocity));
                _previousPositionalDelta = positionalDelta;
                _previousDeltaTimeStep = deltaTimeStep;
            }
        }

        btTransform worldTrans = rigidBody->getWorldTransform();
        worldTrans.setOrigin(glmToBullet(_positionalTarget));
        worldTrans.setRotation(glmToBullet(_rotationalTarget));
        rigidBody->setWorldTransform(worldTrans);

        motionState->dirtyInternalKinematicChanges();

        _previousPositionalTarget = _positionalTarget;
        _previousRotationalTarget = _rotationalTarget;
        _previousSet = true;
    });

    activateBody();
}

bool AvatarActionHold::updateArguments(QVariantMap arguments) {
    glm::vec3 relativePosition;
    glm::quat relativeRotation;
    float timeScale;
    QString hand;
    QUuid holderID;
    bool kinematic;
    bool kinematicSetVelocity;
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

        ok = true;
        kinematic = EntityActionInterface::extractBooleanArgument("hold", arguments, "kinematic", ok, false);
        if (!ok) {
            _kinematic = false;
        }

        ok = true;
        kinematicSetVelocity = EntityActionInterface::extractBooleanArgument("hold", arguments,
                                                                             "kinematicSetVelocity", ok, false);
        if (!ok) {
            _kinematicSetVelocity = false;
        }

        if (somethingChanged ||
            relativePosition != _relativePosition ||
            relativeRotation != _relativeRotation ||
            timeScale != _linearTimeScale ||
            hand != _hand ||
            holderID != _holderID ||
            kinematic != _kinematic ||
            kinematicSetVelocity != _kinematicSetVelocity) {
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
            _holderID = holderID;
            _kinematic = kinematic;
            _kinematicSetVelocity = kinematicSetVelocity;
            _active = true;

            auto ownerEntity = _ownerEntity.lock();
            if (ownerEntity) {
                ownerEntity->setActionDataDirty(true);
                ownerEntity->setActionDataNeedsTransmit(true);
            }
        });
        activateBody();
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
        arguments["kinematic"] = _kinematic;
        arguments["kinematicSetVelocity"] = _kinematicSetVelocity;
    });
    return arguments;
}

QByteArray AvatarActionHold::serialize() const {
    QByteArray serializedActionArguments;
    QDataStream dataStream(&serializedActionArguments, QIODevice::WriteOnly);

    withReadLock([&]{
        dataStream << ACTION_TYPE_HOLD;
        dataStream << getID();
        dataStream << AvatarActionHold::holdVersion;

        dataStream << _holderID;
        dataStream << _relativePosition;
        dataStream << _relativeRotation;
        dataStream << _linearTimeScale;
        dataStream << _hand;

        dataStream << localTimeToServerTime(_expires);
        dataStream << _tag;
        dataStream << _kinematic;
        dataStream << _kinematicSetVelocity;
    });

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

    withWriteLock([&]{
        dataStream >> _holderID;
        dataStream >> _relativePosition;
        dataStream >> _relativeRotation;
        dataStream >> _linearTimeScale;
        _angularTimeScale = _linearTimeScale;
        dataStream >> _hand;

        quint64 serverExpires;
        dataStream >> serverExpires;
        _expires = serverTimeToLocalTime(serverExpires);

        dataStream >> _tag;
        dataStream >> _kinematic;
        dataStream >> _kinematicSetVelocity;

        #if WANT_DEBUG
        qDebug() << "deserialize AvatarActionHold: " << _holderID
                 << _relativePosition.x << _relativePosition.y << _relativePosition.z
                 << _hand << _expires;
        #endif

        _active = true;
    });
}
