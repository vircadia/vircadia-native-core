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
    cacheShapeDiameter();
}

void AvatarMotionState::handleEasyChanges(uint32_t& flags) {
    ObjectMotionState::handleEasyChanges(flags);
    if (flags & Simulation::DIRTY_PHYSICS_ACTIVATION && !_body->isActive()) {
        _body->activate();
    }
}

bool AvatarMotionState::handleHardAndEasyChanges(uint32_t& flags, PhysicsEngine* engine) {
    return ObjectMotionState::handleHardAndEasyChanges(flags, engine);
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
        _body->setAngularVelocity(glmToBullet(getObjectAngularVelocity()));
    }
}

// virtual
void AvatarMotionState::setWorldTransform(const btTransform& worldTrans) {
    const float SPRING_TIMESCALE = 0.5f;
    float tau = PHYSICS_ENGINE_FIXED_SUBSTEP / SPRING_TIMESCALE;
    btVector3 currentPosition = worldTrans.getOrigin();
    btVector3 offsetToTarget = glmToBullet(getObjectPosition()) - currentPosition;
    float distance = offsetToTarget.length();
    if ((1.0f - tau) * distance > _diameter) {
        // the avatar body is far from its target --> slam position
        btTransform newTransform;
        newTransform.setOrigin(currentPosition + offsetToTarget);
        newTransform.setRotation(glmToBullet(getObjectRotation()));
        _body->setWorldTransform(newTransform);
        _body->setLinearVelocity(glmToBullet(getObjectLinearVelocity()));
        _body->setAngularVelocity(glmToBullet(getObjectAngularVelocity()));
    } else {
        // the avatar body is near its target --> slam velocity
        btVector3 velocity = glmToBullet(getObjectLinearVelocity()) + (1.0f / SPRING_TIMESCALE) * offsetToTarget;
        _body->setLinearVelocity(velocity);
        _body->setAngularVelocity(glmToBullet(getObjectAngularVelocity()));
        // slam its rotation
        btTransform newTransform = worldTrans;
        newTransform.setRotation(glmToBullet(getObjectRotation()));
        _body->setWorldTransform(newTransform);
    }
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
    return _avatar->getWorldPosition();
}

// virtual
glm::quat AvatarMotionState::getObjectRotation() const {
    return _avatar->getWorldOrientation();
}

// virtual
glm::vec3 AvatarMotionState::getObjectLinearVelocity() const {
    return _avatar->getWorldVelocity();
}

// virtual
glm::vec3 AvatarMotionState::getObjectAngularVelocity() const {
    // HACK: avatars use a capusle collision shape and their angularVelocity in the local simulation is unimportant.
    // Therefore, as optimization toward support for larger crowds we ignore it and return zero.
    //return _avatar->getWorldAngularVelocity();
    return glm::vec3(0.0f);
}

// virtual
glm::vec3 AvatarMotionState::getObjectGravity() const {
    return std::static_pointer_cast<Avatar>(_avatar)->getAcceleration();
}

// virtual
const QUuid AvatarMotionState::getObjectID() const {
    return _avatar->getSessionUUID();
}

QString AvatarMotionState::getName() const {
    return _avatar->getName();
}

// virtual
QUuid AvatarMotionState::getSimulatorID() const {
    return _avatar->getSessionUUID();
}

// virtual
void AvatarMotionState::computeCollisionGroupAndMask(int32_t& group, int32_t& mask) const {
    group = BULLET_COLLISION_GROUP_OTHER_AVATAR;
    mask = Physics::getDefaultCollisionMask(group);
}

// virtual
float AvatarMotionState::getMass() const {
    return std::static_pointer_cast<Avatar>(_avatar)->computeMass();
}

void AvatarMotionState::cacheShapeDiameter() {
    if (_shape) {
        // shape is capsuleY
        btVector3 aabbMin, aabbMax;
        btTransform transform;
        transform.setIdentity();
        _shape->getAabb(transform, aabbMin, aabbMax);
        _diameter = (aabbMax - aabbMin).getX();
    } else {
        _diameter = 0.0f;
    }
}

void AvatarMotionState::setRigidBody(btRigidBody* body) {
    ObjectMotionState::setRigidBody(body);
    if (_body) {
        // remove angular dynamics from this body
        _body->setAngularFactor(0.0f);
    }
}

void AvatarMotionState::setShape(const btCollisionShape* shape) {
    ObjectMotionState::setShape(shape);
    cacheShapeDiameter();
}
