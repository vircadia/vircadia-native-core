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

#include "AvatarMotionState.h"

#include <PhysicsCollisionGroups.h>
#include <PhysicsEngine.h>
#include <PhysicsHelpers.h>


AvatarMotionState::AvatarMotionState(AvatarSharedPointer avatar, const btCollisionShape* shape) : ObjectMotionState(shape), _avatar(avatar) {
    assert(_avatar);
    _type = MOTIONSTATE_TYPE_AVATAR;
}

AvatarMotionState::~AvatarMotionState() {
    assert(_avatar);
    _avatar = nullptr;
}

// virtual
uint32_t AvatarMotionState::getIncomingDirtyFlags() {
    return _body ? _dirtyFlags : 0;
}

void AvatarMotionState::clearIncomingDirtyFlags() {
    if (_body) {
        _dirtyFlags = 0;
    }
}

PhysicsMotionType AvatarMotionState::computePhysicsMotionType() const {
    // TODO?: support non-DYNAMIC motion for avatars? (e.g. when sitting)
    return MOTION_TYPE_DYNAMIC;
}

// virtual and protected
const btCollisionShape* AvatarMotionState::computeNewShape() {
    ShapeInfo shapeInfo;
    std::static_pointer_cast<Avatar>(_avatar)->computeShapeInfo(shapeInfo);
    return getShapeManager()->getShape(shapeInfo);
}

// virtual
bool AvatarMotionState::isMoving() const {
    return false;
}

// virtual
void AvatarMotionState::getWorldTransform(btTransform& worldTrans) const {
    worldTrans.setOrigin(glmToBullet(getObjectPosition()));
    worldTrans.setRotation(glmToBullet(getObjectRotation()));
    if (_body) {
        _body->setLinearVelocity(glmToBullet(getObjectLinearVelocity()));
        _body->setAngularVelocity(glmToBullet(getObjectLinearVelocity()));
    }
}

// virtual
void AvatarMotionState::setWorldTransform(const btTransform& worldTrans) {
    // HACK: The PhysicsEngine does not actually move OTHER avatars -- instead it slaves their local RigidBody to the transform
    // as specified by a remote simulation.  However, to give the remote simulation time to respond to our own objects we tie
    // the other avatar's body to its true position with a simple spring. This is a HACK that will have to be improved later.
    const float SPRING_TIMESCALE = 0.5f;
    float tau = PHYSICS_ENGINE_FIXED_SUBSTEP / SPRING_TIMESCALE;
    btVector3 currentPosition = worldTrans.getOrigin();
    btVector3 targetPosition = glmToBullet(getObjectPosition());
    btTransform newTransform;
    newTransform.setOrigin((1.0f - tau) * currentPosition + tau * targetPosition);
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
    return _avatar->getPosition();
}

// virtual
glm::quat AvatarMotionState::getObjectRotation() const {
    return _avatar->getOrientation();
}

// virtual
glm::vec3 AvatarMotionState::getObjectLinearVelocity() const {
    return _avatar->getVelocity();
}

// virtual
glm::vec3 AvatarMotionState::getObjectAngularVelocity() const {
    return _avatar->getAngularVelocity();
}

// virtual
glm::vec3 AvatarMotionState::getObjectGravity() const {
    return std::static_pointer_cast<Avatar>(_avatar)->getAcceleration();
}

// virtual
const QUuid AvatarMotionState::getObjectID() const {
    return _avatar->getSessionUUID();
}

// virtual
QUuid AvatarMotionState::getSimulatorID() const {
    return _avatar->getSessionUUID();
}

// virtual
void AvatarMotionState::computeCollisionGroupAndMask(int16_t& group, int16_t& mask) const {
    group = BULLET_COLLISION_GROUP_OTHER_AVATAR;
    mask = Physics::getDefaultCollisionMask(group);
}

