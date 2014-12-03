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

// TODO: store _cachedWorldOffset in a more central location -- VoxelTree and others also need to know about it
// origin of physics simulation in world frame
glm::vec3 _cachedWorldOffset(0.0f);

// static 
void EntityMotionState::setWorldOffset(const glm::vec3& offset) {
    _cachedWorldOffset = offset;
}

// static 
const glm::vec3& getWorldOffset() {
    return _cachedWorldOffset;
}

EntityMotionState::EntityMotionState(EntityItem* entity) : _entity(entity) {
    assert(entity != NULL);
    _oldBoundingCube = _entity->getMaximumAACube();
}

EntityMotionState::~EntityMotionState() {
}

MotionType EntityMotionState::getMotionType() const {
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
    glmToBullet(_entity->getPositionInMeters() - _cachedWorldOffset, pos);
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
        _entity->setPositionInMeters(pos + _cachedWorldOffset);
    
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

