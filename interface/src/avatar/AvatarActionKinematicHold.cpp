//
//  AvatarActionKinematicHold.cpp
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

#include "AvatarActionKinematicHold.h"

const uint16_t AvatarActionKinematicHold::holdVersion = 1;

AvatarActionKinematicHold::AvatarActionKinematicHold(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectActionSpring(id, ownerEntity),
    _relativePosition(glm::vec3(0.0f)),
    _relativeRotation(glm::quat()),
    _hand("right"),
    _mine(false),
    _previousPositionalTarget(Vectors::ZERO),
    _previousRotationalTarget(Quaternions::IDENTITY)
{
    _type = ACTION_TYPE_KINEMATIC_HOLD;
    #if WANT_DEBUG
    qDebug() << "AvatarActionKinematicHold::AvatarActionKinematicHold";
    #endif
}

AvatarActionKinematicHold::~AvatarActionKinematicHold() {
    #if WANT_DEBUG
    qDebug() << "AvatarActionKinematicHold::~AvatarActionKinematicHold";
    #endif
}

void AvatarActionKinematicHold::updateActionWorker(float deltaTimeStep) {
    if (!_mine) {
        // if a local script isn't updating this, then we are just getting spring-action data over the wire.
        // let the super-class handle it.
        ObjectActionSpring::updateActionWorker(deltaTimeStep);
        return;
    }

    assert(deltaTimeStep > 0.0f);

    glm::quat rotation;
    glm::vec3 position;
    glm::vec3 offset;
    bool gotLock = withTryReadLock([&]{
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
            if (_positionalTarget != position || _rotationalTarget != rotation) {
                _positionalTarget = position;
                _rotationalTarget = rotation;
                auto ownerEntity = _ownerEntity.lock();
                if (ownerEntity) {
                    ownerEntity->setActionDataDirty(true);
                    void* physicsInfo = ownerEntity->getPhysicsInfo();
                    if (physicsInfo) {
                        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
                        btRigidBody* rigidBody = motionState ? motionState->getRigidBody() : nullptr;
                        if (!rigidBody) {
                            qDebug() << "ObjectActionSpring::updateActionWorker no rigidBody";
                            return;
                        }

                        if (_setVelocity) {
                            if (_previousSet) {
                                glm::vec3 positionalVelocity = (_positionalTarget - _previousPositionalTarget) / deltaTimeStep;
                                rigidBody->setLinearVelocity(glmToBullet(positionalVelocity));
                                // back up along velocity a bit in order to smooth out a "vibrating" appearance
                                _positionalTarget -= positionalVelocity * deltaTimeStep / 2.0f;
                            }
                        }

                        btTransform worldTrans = rigidBody->getWorldTransform();
                        worldTrans.setOrigin(glmToBullet(_positionalTarget));
                        worldTrans.setRotation(glmToBullet(_rotationalTarget));
                        rigidBody->setWorldTransform(worldTrans);

                        _previousPositionalTarget = _positionalTarget;
                        _previousRotationalTarget = _rotationalTarget;
                        _previousSet = true;
                    }
                }
            }
        });
    }

    if (gotLock) {
        ObjectActionSpring::updateActionWorker(deltaTimeStep);
    }
}


bool AvatarActionKinematicHold::updateArguments(QVariantMap arguments) {
    if (!ObjectAction::updateArguments(arguments)) {
        return false;
    }
    bool ok = true;
    glm::vec3 relativePosition =
        EntityActionInterface::extractVec3Argument("kinematic-hold", arguments, "relativePosition", ok, false);
    if (!ok) {
        relativePosition = _relativePosition;
    }

    ok = true;
    glm::quat relativeRotation =
        EntityActionInterface::extractQuatArgument("kinematic-hold", arguments, "relativeRotation", ok, false);
    if (!ok) {
        relativeRotation = _relativeRotation;
    }

    ok = true;
    QString hand =
        EntityActionInterface::extractStringArgument("kinematic-hold", arguments, "hand", ok, false);
    if (!ok || !(hand == "left" || hand == "right")) {
        hand = _hand;
    }

    ok = true;
    int setVelocity =
        EntityActionInterface::extractIntegerArgument("kinematic-hold", arguments, "setVelocity", ok, false);
    if (!ok) {
        setVelocity = false;
    }

    if (relativePosition != _relativePosition
            || relativeRotation != _relativeRotation
            || hand != _hand) {
        withWriteLock([&] {
            _relativePosition = relativePosition;
            _relativeRotation = relativeRotation;
            _hand = hand;
            _setVelocity = setVelocity;

            _mine = true;
            _active = true;
            activateBody();
        });
    }
    return true;
}


QVariantMap AvatarActionKinematicHold::getArguments() {
    QVariantMap arguments = ObjectAction::getArguments();
    withReadLock([&]{
        if (!_mine) {
            arguments = ObjectActionSpring::getArguments();
            return;
        }

        arguments["relativePosition"] = glmToQMap(_relativePosition);
        arguments["relativeRotation"] = glmToQMap(_relativeRotation);
        arguments["setVelocity"] = _setVelocity;
        arguments["hand"] = _hand;
    });
    return arguments;
}


void AvatarActionKinematicHold::deserialize(QByteArray serializedArguments) {
    if (!_mine) {
        ObjectActionSpring::deserialize(serializedArguments);
    }
}
