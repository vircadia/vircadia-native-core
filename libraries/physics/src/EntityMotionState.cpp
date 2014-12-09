//
//  EntityMotionState.cpp
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2014.11.06
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <EntityItem.h>

#ifdef USE_BULLET_PHYSICS
#include "BulletUtil.h"
#endif // USE_BULLET_PHYSICS
#include "EntityMotionState.h"

// TODO: store _worldOffset in a more central location -- VoxelTree and others also need to know about it
// origin of physics simulation in world frame
glm::vec3 _worldOffset(0.0f);

// static 
void EntityMotionState::setWorldOffset(const glm::vec3& offset) {
    _worldOffset = offset;
}

// static 
const glm::vec3& getWorldOffset() {
    return _worldOffset;
}

EntityMotionState::EntityMotionState(EntityItem* entity) 
    :   _entity(entity),
        _sentMoving(false),
        _notMoving(true),
        _recievedNotMoving(false),
        _sentFrame(0),
        _sentPosition(0.0f),
        _sentRotation(),
        _sentVelocity(0.0f),
        _sentVelocity(0.0f),
        _sentAngularVelocity(0.0f),
        _sentGravity(0.0f)
{
    assert(entity != NULL);
    _oldBoundingCube = _entity->getMaximumAACube();
}

EntityMotionState::~EntityMotionState() {
}

MotionType EntityMotionState::computeMotionType() const {
    // HACK: According to EntityTree the meaning of "static" is "not moving" whereas
    // to Bullet it means "can't move".  For demo purposes we temporarily interpret
    // Entity::weightless to mean Bullet::static.
    return _entity->getCollisionsWillMove() ? MOTION_TYPE_DYNAMIC : MOTION_TYPE_STATIC;
}

#ifdef USE_BULLET_PHYSICS
// This callback is invoked by the physics simulation in two cases:
// (1) when the RigidBody is first added to the world
//     (irregardless of MotionType: STATIC, DYNAMIC, or KINEMATIC)
// (2) at the beginning of each simulation frame for KINEMATIC RigidBody's --
//     it is an opportunity for outside code to update the object's simulation position
void EntityMotionState::getWorldTransform (btTransform &worldTrans) const {
    btVector3 pos;
    glmToBullet(_entity->getPositionInMeters() - _worldOffset, pos);
    worldTrans.setOrigin(pos);

    btQuaternion rot;
    glmToBullet(_entity->getRotation(), rot);
    worldTrans.setRotation(rot);

    applyVelocities();
}

// This callback is invoked by the physics simulation at the end of each simulation frame...
// iff the corresponding RigidBody is DYNAMIC and has moved.
void EntityMotionState::setWorldTransform (const btTransform &worldTrans) {
    uint32_t updateFlags = _entity->getUpdateFlags();
    if (! (updateFlags &  EntityItem::UPDATE_POSITION)) {
        glm::vec3 pos;
        bulletToGLM(worldTrans.getOrigin(), pos);
        _entity->setPositionInMeters(pos + _worldOffset);
    
        glm::quat rot;
        bulletToGLM(worldTrans.getRotation(), rot);
        _entity->setRotation(rot);
    }

    if (! (updateFlags &  EntityItem::UPDATE_VELOCITY)) {
        glm::vec3 v;
        getVelocity(v);
        _entity->setVelocityInMeters(v);
        getAngularVelocity(v);
        _entity->setAngularVelocity(v);
    }
}
#endif // USE_BULLET_PHYSICS

void EntityMotionState::applyVelocities() const {
#ifdef USE_BULLET_PHYSICS
    if (_body) {
        setVelocity(_entity->getVelocityInMeters());
        setAngularVelocity(_entity->getAngularVelocity());
        _body->setActivationState(ACTIVE_TAG);
    }
#endif // USE_BULLET_PHYSICS
}

void EntityMotionState::applyGravity() const {
#ifdef USE_BULLET_PHYSICS
    if (_body) {
        setGravity(_entity->getGravityInMeters());
        _body->setActivationState(ACTIVE_TAG);
    }
#endif // USE_BULLET_PHYSICS
}

void EntityMotionState::computeShapeInfo(ShapeInfo& info) {
    _entity->computeShapeInfo(info);
}

void EntityMotionState::getBoundingCubes(AACube& oldCube, AACube& newCube) {
    oldCube = _oldBoundingCube;
    newCube = _entity->getMaximumAACube();
    _oldBoundingCube = newCube;
}

const float FIXED_SUBSTEP = 1.0f / 60.0f;
bool EntityMotionState::shouldSendUpdate(uint32_t simulationFrame, float subStepRemainder) const {
    // TODO: Andrew to test this and make sure it works as expected
    assert(_body);
    float dt = (float)(simulationFrame - _sentFrame) * FIXED_SUBSTEP + subStepRemainder;
    const float DEFAULT_UPDATE_PERIOD = 10.0f;j
    if (dt > DEFAULT_UPDATE_PERIOD) {
        return ! (_notMoving && _recievedNotMoving);
    }
    if (_sentMoving && _notMoving) {
        return true;
    }

    // compute position error
    glm::vec3 expectedPosition = _sentPosition + dt * (_sentVelocity + (0.5f * dt) * _sentGravity);

    glm::vec3 actualPos;
    btTransform worldTrans = _body->getWorldTransform();
    bulletToGLM(worldTrans.getOrigin(), actualPos);
    
    float dx2 = glm::length2(actualPosition - expectedPosition);
    const MAX_POSITION_ERROR_SQUARED = 0.001; // 0.001 m^2 ~~> 0.03 m
    if (dx2 > MAX_POSITION_ERROR_SQUARED) {
        return true;
    }

    // compute rotation error
    float spin = glm::length(_sentAngularVelocity);
    glm::quat expectedRotation = _sentRotation;
    const float MIN_SPIN = 1.0e-4f;
    if (spin > MIN_SPIN) {
        glm::vec3 axis = _sentAngularVelocity / spin;
        expectedRotation = glm::angleAxis(dt * spin, axis) * _sentRotation;
    }
    const float MIN_ROTATION_DOT = 0.98f;
    glm::quat actualRotation;
    bulletToGLM(worldTrans.getRotation(), actualRotation);
    return (glm::dot(actualRotation, expectedRotation) < MIN_ROTATION_DOT);
}
