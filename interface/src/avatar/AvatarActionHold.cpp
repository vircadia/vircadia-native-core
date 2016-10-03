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

#include "avatar/AvatarManager.h"
#include "CharacterController.h"

const uint16_t AvatarActionHold::holdVersion = 1;
const int AvatarActionHold::velocitySmoothFrames = 6;


AvatarActionHold::AvatarActionHold(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectActionSpring(id, ownerEntity)
{
    _type = ACTION_TYPE_HOLD;
    _measuredLinearVelocities.resize(AvatarActionHold::velocitySmoothFrames);

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    if (myAvatar) {
        myAvatar->addHoldAction(this);
    }

#if WANT_DEBUG
    qDebug() << "AvatarActionHold::AvatarActionHold" << (void*)this;
#endif
}

AvatarActionHold::~AvatarActionHold() {
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    if (myAvatar) {
        myAvatar->removeHoldAction(this);
    }

#if WANT_DEBUG
    qDebug() << "AvatarActionHold::~AvatarActionHold" << (void*)this;
#endif
}

bool AvatarActionHold::getAvatarRigidBodyLocation(glm::vec3& avatarRigidBodyPosition, glm::quat& avatarRigidBodyRotation) {
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    MyCharacterController* controller = myAvatar ? myAvatar->getCharacterController() : nullptr;
    if (!controller) {
        qDebug() << "AvatarActionHold::getAvatarRigidBodyLocation failed to get character controller";
        return false;
    }
    controller->getRigidBodyLocation(avatarRigidBodyPosition, avatarRigidBodyRotation);
    return true;
}

void AvatarActionHold::prepareForPhysicsSimulation() {
    auto avatarManager = DependencyManager::get<AvatarManager>();
    auto holdingAvatar = std::static_pointer_cast<Avatar>(avatarManager->getAvatarBySessionID(_holderID));

    if (!holdingAvatar || !holdingAvatar->isMyAvatar()) {
        return;
    }

    withWriteLock([&]{
        glm::vec3 avatarRigidBodyPosition;
        glm::quat avatarRigidBodyRotation;
        getAvatarRigidBodyLocation(avatarRigidBodyPosition, avatarRigidBodyRotation);

        if (_ignoreIK) {
            return;
        }

        glm::vec3 palmPosition;
        glm::quat palmRotation;
        if (_hand == "right") {
            palmPosition = holdingAvatar->getUncachedRightPalmPosition();
            palmRotation = holdingAvatar->getUncachedRightPalmRotation();
        } else {
            palmPosition = holdingAvatar->getUncachedLeftPalmPosition();
            palmRotation = holdingAvatar->getUncachedLeftPalmRotation();
        }


        // determine the difference in translation and rotation between the avatar's
        // rigid body and the palm position.  The avatar's rigid body will be moved by bullet
        // between this call and the call to getTarget, below.  A call to get*PalmPosition in
        // getTarget would get the palm position of the previous location of the avatar (because
        // bullet has moved the av's rigid body but the rigid body's location has not yet been
        // copied out into the Avatar class.
        //glm::quat avatarRotationInverse = glm::inverse(avatarRigidBodyRotation);

        // the offset should be in the frame of the avatar, but something about the order
        // things are updated makes this wrong:
        //   _palmOffsetFromRigidBody = avatarRotationInverse * (palmPosition - avatarRigidBodyPosition);
        // I'll leave it here as a comment in case avatar handling changes.
        _palmOffsetFromRigidBody = palmPosition - avatarRigidBodyPosition;

        // rotation should also be needed, but again, the order of updates makes this unneeded.  leaving
        // code here for future reference.
        // _palmRotationFromRigidBody = avatarRotationInverse * palmRotation;
    });

    activateBody(true);
}

bool AvatarActionHold::getTarget(float deltaTimeStep, glm::quat& rotation, glm::vec3& position,
                                 glm::vec3& linearVelocity, glm::vec3& angularVelocity) {
    auto avatarManager = DependencyManager::get<AvatarManager>();
    auto holdingAvatar = std::static_pointer_cast<Avatar>(avatarManager->getAvatarBySessionID(_holderID));

    if (!holdingAvatar) {
        return false;;
    }

    withReadLock([&]{
        bool isRightHand = (_hand == "right");

        glm::vec3 palmPosition;
        glm::quat palmRotation;

        if (holdingAvatar->isMyAvatar()) {

            // fetch the hand controller pose
            controller::Pose pose;
            if (isRightHand) {
                pose = avatarManager->getMyAvatar()->getRightHandControllerPoseInWorldFrame();
            } else {
                pose = avatarManager->getMyAvatar()->getLeftHandControllerPoseInWorldFrame();
            }

            if (pose.isValid()) {
                linearVelocity = pose.getVelocity();
                angularVelocity = pose.getAngularVelocity();

                if (isRightHand) {
                    pose = avatarManager->getMyAvatar()->getRightHandControllerPoseInAvatarFrame();
                } else {
                    pose = avatarManager->getMyAvatar()->getLeftHandControllerPoseInAvatarFrame();
                }
            }

            if (_ignoreIK && pose.isValid()) {
                Transform avatarTransform;
                auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
                avatarTransform = myAvatar->getTransform();
                palmPosition = avatarTransform.transform(pose.getTranslation() / myAvatar->getTargetScale());
                palmRotation = avatarTransform.getRotation() * pose.getRotation();
            } else {
                glm::vec3 avatarRigidBodyPosition;
                glm::quat avatarRigidBodyRotation;
                getAvatarRigidBodyLocation(avatarRigidBodyPosition, avatarRigidBodyRotation);

                // the offset and rotation between the avatar's rigid body and the palm were determined earlier
                // in prepareForPhysicsSimulation.  At this point, the avatar's rigid body has been moved by bullet
                // and the data in the Avatar class is stale.  This means that the result of get*PalmPosition will
                // be stale.  Instead, determine the current palm position with the current avatar's rigid body
                // location and the saved offsets.

                // this line is more correct but breaks for the current way avatar data is updated.
                // palmPosition = avatarRigidBodyPosition + avatarRigidBodyRotation * _palmOffsetFromRigidBody;
                // instead, use this for now:
                palmPosition = avatarRigidBodyPosition + _palmOffsetFromRigidBody;

                // the item jitters the least by getting the rotation based on the opinion of Avatar.h rather
                // than that of the rigid body.  leaving this next line here for future reference:
                // palmRotation = avatarRigidBodyRotation * _palmRotationFromRigidBody;

                if (isRightHand) {
                    palmRotation = holdingAvatar->getRightPalmRotation();
                } else {
                    palmRotation = holdingAvatar->getLeftPalmRotation();
                }
            }
        } else { // regular avatar
            if (isRightHand) {
                Transform controllerRightTransform = Transform(holdingAvatar->getControllerRightHandMatrix());
                Transform avatarTransform = holdingAvatar->getTransform();
                palmRotation = avatarTransform.getRotation() * controllerRightTransform.getRotation();
                palmPosition = avatarTransform.getTranslation() +
                    (avatarTransform.getRotation() * controllerRightTransform.getTranslation());
            } else {
                Transform controllerLeftTransform = Transform(holdingAvatar->getControllerLeftHandMatrix());
                Transform avatarTransform = holdingAvatar->getTransform();
                palmRotation = avatarTransform.getRotation() * controllerLeftTransform.getRotation();
                palmPosition = avatarTransform.getTranslation() +
                    (avatarTransform.getRotation() * controllerLeftTransform.getTranslation());
            }
        }

        rotation = palmRotation * _relativeRotation;
        position = palmPosition + rotation * _relativePosition;

        // update linearVelocity based on offset via _relativePosition;
        linearVelocity = linearVelocity + glm::cross(angularVelocity, position - palmPosition);
    });

    return true;
}

void AvatarActionHold::updateActionWorker(float deltaTimeStep) {
    if (_kinematic) {
        if (prepareForSpringUpdate(deltaTimeStep)) {
            doKinematicUpdate(deltaTimeStep);
        }
    } else {
        forceBodyNonStatic();
        ObjectActionSpring::updateActionWorker(deltaTimeStep);
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
        if (_previousSet &&
            _positionalTarget != _previousPositionalTarget) { // don't average in a zero velocity if we get the same data
            glm::vec3 oneFrameVelocity = (_positionalTarget - _previousPositionalTarget) / deltaTimeStep;

            _measuredLinearVelocities[_measuredLinearVelocitiesIndex++] = oneFrameVelocity;
            if (_measuredLinearVelocitiesIndex >= AvatarActionHold::velocitySmoothFrames) {
                _measuredLinearVelocitiesIndex = 0;
            }
        }

        glm::vec3 measuredLinearVelocity;
        for (int i = 0; i < AvatarActionHold::velocitySmoothFrames; i++) {
            // there is a bit of lag between when someone releases the trigger and when the software reacts to
            // the release.  we calculate the velocity from previous frames but we don't include several
            // of the most recent.
            //
            // if _measuredLinearVelocitiesIndex is
            //     0 -- ignore i of 3 4 5
            //     1 -- ignore i of 4 5 0
            //     2 -- ignore i of 5 0 1
            //     3 -- ignore i of 0 1 2
            //     4 -- ignore i of 1 2 3
            //     5 -- ignore i of 2 3 4

            // This code is now disabled, but I'm leaving it commented-out because I suspect it will come back.
            // if ((i + 1) % AvatarActionHold::velocitySmoothFrames == _measuredLinearVelocitiesIndex ||
            //     (i + 2) % AvatarActionHold::velocitySmoothFrames == _measuredLinearVelocitiesIndex ||
            //     (i + 3) % AvatarActionHold::velocitySmoothFrames == _measuredLinearVelocitiesIndex) {
            //     continue;
            // }

            measuredLinearVelocity += _measuredLinearVelocities[i];
        }
        measuredLinearVelocity /= (float)(AvatarActionHold::velocitySmoothFrames
                                          // - 3  // 3 because of the 3 we skipped, above
                                          );

        if (_kinematicSetVelocity) {
            rigidBody->setLinearVelocity(glmToBullet(measuredLinearVelocity));
            rigidBody->setAngularVelocity(glmToBullet(_angularVelocityTarget));
        }

        btTransform worldTrans = rigidBody->getWorldTransform();
        worldTrans.setOrigin(glmToBullet(_positionalTarget));
        worldTrans.setRotation(glmToBullet(_rotationalTarget));
        rigidBody->setWorldTransform(worldTrans);

        motionState->dirtyInternalKinematicChanges();

        _previousPositionalTarget = _positionalTarget;
        _previousRotationalTarget = _rotationalTarget;
        _previousDeltaTimeStep = deltaTimeStep;
        _previousSet = true;
    });

    forceBodyNonStatic();
    activateBody(true);
}

bool AvatarActionHold::updateArguments(QVariantMap arguments) {
    glm::vec3 relativePosition;
    glm::quat relativeRotation;
    float timeScale;
    QString hand;
    QUuid holderID;
    bool kinematic;
    bool kinematicSetVelocity;
    bool ignoreIK;
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

        auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        holderID = myAvatar->getSessionUUID();

        ok = true;
        kinematic = EntityActionInterface::extractBooleanArgument("hold", arguments, "kinematic", ok, false);
        if (!ok) {
            kinematic = _kinematic;
        }

        ok = true;
        kinematicSetVelocity = EntityActionInterface::extractBooleanArgument("hold", arguments,
                                                                             "kinematicSetVelocity", ok, false);
        if (!ok) {
            kinematicSetVelocity = _kinematicSetVelocity;
        }

        ok = true;
        ignoreIK = EntityActionInterface::extractBooleanArgument("hold", arguments, "ignoreIK", ok, false);
        if (!ok) {
            ignoreIK = _ignoreIK;
        }

        if (somethingChanged ||
            relativePosition != _relativePosition ||
            relativeRotation != _relativeRotation ||
            timeScale != _linearTimeScale ||
            hand != _hand ||
            holderID != _holderID ||
            kinematic != _kinematic ||
            kinematicSetVelocity != _kinematicSetVelocity ||
            ignoreIK != _ignoreIK) {
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
            _ignoreIK = ignoreIK;
            _active = true;

            auto ownerEntity = _ownerEntity.lock();
            if (ownerEntity) {
                ownerEntity->setActionDataDirty(true);
                ownerEntity->setActionDataNeedsTransmit(true);
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
        arguments["kinematic"] = _kinematic;
        arguments["kinematicSetVelocity"] = _kinematicSetVelocity;
        arguments["ignoreIK"] = _ignoreIK;
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

    forceBodyNonStatic();
}

void AvatarActionHold::lateAvatarUpdate(const AnimPose& prePhysicsRoomPose, const AnimPose& postAvatarUpdateRoomPose) {
    auto ownerEntity = _ownerEntity.lock();
    if (!ownerEntity) {
        return;
    }
    void* physicsInfo = ownerEntity->getPhysicsInfo();
    if (!physicsInfo) {
        return;
    }
    ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
    btRigidBody* rigidBody = motionState ? motionState->getRigidBody() : nullptr;
    if (!rigidBody) {
        return;
    }
    auto avatarManager = DependencyManager::get<AvatarManager>();
    auto holdingAvatar = std::static_pointer_cast<Avatar>(avatarManager->getAvatarBySessionID(_holderID));
    if (!holdingAvatar || !holdingAvatar->isMyAvatar()) {
        return;
    }

    btTransform worldTrans = rigidBody->getWorldTransform();
    AnimPose worldBodyPose(glm::vec3(1), bulletToGLM(worldTrans.getRotation()), bulletToGLM(worldTrans.getOrigin()));

    // transform the body transform into sensor space with the prePhysics sensor-to-world matrix.
    // then transform it back into world uisng the postAvatarUpdate sensor-to-world matrix.
    AnimPose newWorldBodyPose = postAvatarUpdateRoomPose * prePhysicsRoomPose.inverse() * worldBodyPose;

    worldTrans.setOrigin(glmToBullet(newWorldBodyPose.trans));
    worldTrans.setRotation(glmToBullet(newWorldBodyPose.rot));
    rigidBody->setWorldTransform(worldTrans);

    bool positionSuccess;
    ownerEntity->setPosition(bulletToGLM(worldTrans.getOrigin()) + ObjectMotionState::getWorldOffset(), positionSuccess, false);
    bool orientationSuccess;
    ownerEntity->setOrientation(bulletToGLM(worldTrans.getRotation()), orientationSuccess, false);
}
