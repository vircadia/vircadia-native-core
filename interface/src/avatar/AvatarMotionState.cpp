//
//  AvatarMotionState.cpp
//  interface/src/avatar/
//
//  Created by Andrew Meadows 2015.05.14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Avatar.h"
#include "AvatarMotionState.h"
#include "BulletUtil.h"

AvatarMotionState::AvatarMotionState(Avatar* avatar, btCollisionShape* shape) : ObjectMotionState(shape), _avatar(avatar) {
}

AvatarMotionState::~AvatarMotionState() {
    _avatar = nullptr;
}

// virtual
uint32_t AvatarMotionState::getAndClearIncomingDirtyFlags() {
    uint32_t dirtyFlags = 0;
    if (_body && _avatar) {
        dirtyFlags = _dirtyFlags;
        _dirtyFlags = 0;
    }
    return dirtyFlags;
}

MotionType AvatarMotionState::computeObjectMotionType() const {
    // TODO?: support non-DYNAMIC motion for avatars? (e.g. when sitting)
    return MOTION_TYPE_DYNAMIC;
}

// virtual
btCollisionShape* AvatarMotionState::computeNewShape() {
    if (_avatar) {
        ShapeInfo shapeInfo;
        _avatar->computeShapeInfo(shapeInfo);
        return getShapeManager()->getShape(shapeInfo);
    }
    return nullptr;
}

// virtual
bool AvatarMotionState::isMoving() const {
    return false;
}

// virtual
void AvatarMotionState::getWorldTransform(btTransform& worldTrans) const {
    if (!_avatar) {
        return;
    }
    worldTrans.setOrigin(glmToBullet(getObjectPosition()));
    worldTrans.setRotation(glmToBullet(getObjectRotation()));
    _body->setLinearVelocity(glmToBullet(getObjectLinearVelocity()));
    _body->setAngularVelocity(glmToBullet(getObjectLinearVelocity()));
}

// virtual 
void AvatarMotionState::setWorldTransform(const btTransform& worldTrans) {
    if (!_avatar) {
        return;
    }
    // The PhysicsEngine does not move other Avatars.  
    // Instead we enforce the current transform of the Avatar
    btTransform newTransform;
    newTransform.setOrigin(glmToBullet(getObjectPosition()));
    newTransform.setRotation(glmToBullet(getObjectRotation()));
    _body->setWorldTransform(newTransform);
    _body->setLinearVelocity(glmToBullet(getObjectLinearVelocity()));
    _body->setAngularVelocity(glmToBullet(getObjectLinearVelocity()));
}

// These pure virtual methods must be implemented for each MotionState type
// and make it possible to implement more complicated methods in this base class.

// virtual
float AvatarMotionState::getObjectRestitution() const {
    return 0.5f;
}

// virtual
float AvatarMotionState::getObjectFriction() const {
    return 0.5f;
}

// virtual
float AvatarMotionState::getObjectLinearDamping() const {
    return 0.5f;
}

// virtual
float AvatarMotionState::getObjectAngularDamping() const {
    return 0.5f;
}

// virtual
glm::vec3 AvatarMotionState::getObjectPosition() const {
    return glm::vec3(0.0f);
}

// virtual
glm::quat AvatarMotionState::getObjectRotation() const {
    return _avatar->getOrientation();
}

// virtual
const glm::vec3& AvatarMotionState::getObjectLinearVelocity() const {
    return _avatar->getVelocity();
}

// virtual
const glm::vec3& AvatarMotionState::getObjectAngularVelocity() const {
    return _avatar->getAngularVelocity();
}

// virtual
const glm::vec3& AvatarMotionState::getObjectGravity() const {
    return _avatar->getAcceleration();
}

// virtual
const QUuid& AvatarMotionState::getObjectID() const {
    return _avatar->getSessionUUID();
}

// virtual
QUuid AvatarMotionState::getSimulatorID() const {
    return _avatar->getSessionUUID();
}

// virtual
void AvatarMotionState::bump() {
}

// virtual 
void AvatarMotionState::clearObjectBackPointer() {
    ObjectMotionState::clearObjectBackPointer();
    _avatar = nullptr;
}


