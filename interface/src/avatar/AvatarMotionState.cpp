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

AvatarMotionState::AvatarMotionState(Avatar* avatar, btCollisionShape* shape) : ObjectMotionState(shape), _avatar(avatar) {
}

AvatarMotionState::~AvatarMotionState() {
    _avatar = nullptr;
}

// virtual
void AvatarMotionState::handleEasyChanges(uint32_t flags) {
}

// virtual
void AvatarMotionState::handleHardAndEasyChanges(uint32_t flags, PhysicsEngine* engine) {
}

// virtual
void AvatarMotionState::updateBodyMaterialProperties() {
}

// virtual
void AvatarMotionState::updateBodyVelocities() {
}

// virtual
uint32_t AvatarMotionState::getAndClearIncomingDirtyFlags() const {
    return 0;
}

// virtual
MotionType AvatarMotionState::computeObjectMotionType() const {
    return _motionType;
}

// virtual
void AvatarMotionState::computeObjectShapeInfo(ShapeInfo& shapeInfo) {
}

// virtual
bool AvatarMotionState::isMoving() const {
    return false;
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

// protected, virtual 
void AvatarMotionState::setMotionType(MotionType motionType) {
}
