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

#include "AvatarActionHold.h"

#include <QVariantGLM.h>

#include "avatar/MyAvatar.h"
#include "avatar/AvatarManager.h"

const uint16_t AvatarActionHold::holdVersion = 1;

AvatarActionHold::AvatarActionHold(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectActionSpring(id, ownerEntity)
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
            glm::vec3 palmPosition;
            glm::quat palmRotation;
            
            bool ignoreIK = true;
            if (ignoreIK) {
                if (_hand == "right") {
                    palmPosition = holdingAvatar->getHand()->getCopyOfPalmData(HandData::RightHand).getPosition();
                    palmRotation = holdingAvatar->getHand()->getCopyOfPalmData(HandData::RightHand).getRotation();
                } else {
                    palmPosition = holdingAvatar->getHand()->getCopyOfPalmData(HandData::LeftHand).getPosition();
                    palmRotation = holdingAvatar->getHand()->getCopyOfPalmData(HandData::LeftHand).getRotation();
                }
            } else {
                if (_hand == "right") {
                    palmPosition = holdingAvatar->getRightPalmPosition();
                    palmRotation = holdingAvatar->getRightPalmRotation();
                } else {
                    palmPosition = holdingAvatar->getLeftPalmPosition();
                    palmRotation = holdingAvatar->getLeftPalmRotation();
                }
            }
            /*
             static const float CONTROLLER_LENGTH_OFFSET = 0.0762f;  // three inches
             static const glm::vec3 CONTROLLER_OFFSET = glm::vec3(CONTROLLER_LENGTH_OFFSET / 2.0f,
             CONTROLLER_LENGTH_OFFSET / 2.0f,
             CONTROLLER_LENGTH_OFFSET * 2.0f);
             static const glm::quat yFlip = glm::angleAxis(PI, Vectors::UNIT_Y);
             static const glm::quat quarterX = glm::angleAxis(PI_OVER_TWO, Vectors::UNIT_X);
             static const glm::quat viveToHand = yFlip * quarterX;
             
             static const glm::quat leftQuaterZ = glm::angleAxis(-PI_OVER_TWO, Vectors::UNIT_Z);
             static const glm::quat rightQuaterZ = glm::angleAxis(PI_OVER_TWO, Vectors::UNIT_Z);
             static const glm::quat eighthX = glm::angleAxis(PI / 4.0f, Vectors::UNIT_X);
             
             static const glm::quat leftRotationOffset = glm::inverse(leftQuaterZ * eighthX) * viveToHand;
             static const glm::quat rightRotationOffset = glm::inverse(rightQuaterZ * eighthX) * viveToHand;
             
             static const glm::vec3 leftTranslationOffset = glm::vec3(-1.0f, 1.0f, 1.0f) * CONTROLLER_OFFSET;
             static const glm::vec3 rightTranslationOffset = CONTROLLER_OFFSET;
             
             glm::quat relRot = glm::inverse(rightRotationOffset);
             glm::vec3 relPos = -rightTranslationOffset;
             relRot = palmRotation * relRot;
             relPos = palmPosition + relRot * relPos;
             
             rotation = relRot * _relativeRotation;
             position = relPos + rotation * _relativePosition;
             glm::quat diffRot = glm::inverse(palmRotation) * rotation;
             glm::vec3 diffPos = glm::inverse(rotation) * (position - palmPosition);
             qDebug() << "diffRot =" << safeEulerAngles(diffRot);
             qDebug() << "diffPos =" << diffPos;
             */
            //[11 / 09 17:40 : 14] [DEBUG] diffRot = { type = 'glm::vec3', x = 3.14159, y = 9.04283e-09, z = 0.785398 }
            //[11 / 09 17:40 : 14][DEBUG] diffPos = { type = 'glm::vec3', x = 1.0524, y = -0.0381001, z = -0.0380997 }
            //_relativeRotation = glm::quat(glm::vec3(3.14159f, 0.0f, 0.785398));
            //_relativePosition = glm::vec3(1.0524, -0.0381001, -0.0380997);
            
            rotation = palmRotation * _relativeRotation;
            position = palmPosition + rotation * _relativePosition;
        }
    });
    
    if (holdingAvatar && gotLock) {
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
