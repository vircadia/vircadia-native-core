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
#include <EntityEditPacketSender.h>

#ifdef USE_BULLET_PHYSICS
#include "BulletUtil.h"
#endif // USE_BULLET_PHYSICS
#include "EntityMotionState.h"

EntityMotionState::EntityMotionState(EntityItem* entity) 
    :   _entity(entity),
        _outgoingPhysicsDirtyFlags(0) {
    assert(entity != NULL);
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
    glmToBullet(_entity->getPositionInMeters() - ObjectMotionState::getWorldOffset(), pos);
    worldTrans.setOrigin(pos);

    btQuaternion rot;
    glmToBullet(_entity->getRotation(), rot);
    worldTrans.setRotation(rot);
}

// This callback is invoked by the physics simulation at the end of each simulation frame...
// iff the corresponding RigidBody is DYNAMIC and has moved.
void EntityMotionState::setWorldTransform (const btTransform &worldTrans) {
    glm::vec3 pos;
    bulletToGLM(worldTrans.getOrigin(), pos);
    _entity->setPositionInMeters(pos + ObjectMotionState::getWorldOffset());

    glm::quat rot;
    bulletToGLM(worldTrans.getRotation(), rot);
    _entity->setRotation(rot);

    glm::vec3 v;
    getVelocity(v);
    _entity->setVelocityInMeters(v);

    getAngularVelocity(v);
    _entity->setAngularVelocity(v);

    _outgoingPacketFlags = DIRTY_PHYSICS_FLAGS;

    ObjectMotionState::enqueueOutgoingPacket(this);
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

void EntityMotionState::sendUpdate(OctreeEditPacketSender* packetSender) {
#ifdef USE_BULLET_PHYSICS
    if (_outgoingPhysicsDirtyFlags) {
        EntityItemProperties properties = _entity->getProperties();

        if (_outgoingPhysicsDirtyFlags == OUTGOING_DIRTY_PHYSICS_FLAGS) {
            // common case: physics engine has changed object position/velocity

            btTransform worldTrans = _body->getWorldTransform();
            bulletToGLM(worldTrans.getOrigin(), _sentPosition);
            properties.setPosition(_sentPosition + ObjectMotionState::getWorldOffset());
        
            bulletToGLM(worldTrans.getRotation(), _sentRotation);
            properties.setRotation(_sentRotation);
        
            if (_body->isActive()) {
                bulletToGLM(_body->getLinearVelocity(), _sentVelocity);
                bulletToGLM(_body->getAngularVelocity(), _sentAngularVelocity);
                bulletToGLM(_body->getGravity(), _sentAcceleration);
            } else {
                _sentVelocity = _sentAngularVelocity = _sentAcceleration = glm::vec3(0.0f);
            }
            properties.setVelocity(_sentVelocity);
            properties.setGravity(_sentAcceleration);
            properties.setAngularVelocity(_sentAngularVelocity);
        } else {
            // uncommon case: physics engine change collided with incoming external change
            // we only send data for flags that are set
    
            if (_outgoingPhysicsDirtyFlags & EntityItem::DIRTY_POSITION) {
                btTransform worldTrans = _body->getWorldTransform();
                bulletToGLM(worldTrans.getOrigin(), _sentPosition);
                properties.setPosition(_sentPosition + ObjectMotionState::getWorldOffset());
            
                bulletToGLM(worldTrans.getRotation(), _sentRotation);
                properties.setRotation(_sentRotation);
            }
        
            if (_outgoingPhysicsDirtyFlags & EntityItem::DIRTY_VELOCITY) {
                if (_body->isActive()) {
                    bulletToGLM(_body->getLinearVelocity(), _sentVelocity);
                    bulletToGLM(_body->getAngularVelocity(), _sentAngularVelocity);
                    bulletToGLM(_body->getGravity(), _sentAcceleration);
                } else {
                    _sentVelocity = _sentAngularVelocity = _sentAcceleration = glm::vec3(0.0f);
                }
                properties.setVelocity(_sentVelocity);
                properties.setAngularVelocity(_sentAngularVelocity);
                properties.setGravity(_sentAcceleration);
            }
        }
        // TODO: Figure out what LastEdited is used for...
        //properties.setLastEdited(now);

        glm::vec3 zero(0.0f);
        _sentMoving = (_sentVelocity == zero && _sentAngularVelocity == zero && _sentAcceleration == zero);

        EntityItemID id(_entity->getID());
        EntityEditPacketSender* entityPacketSender = static_cast<EntityEditPacketSender*>(packetSender);
        entityPacketSender->queueEditEntityMessage(PacketTypeEntityAddOrEdit, id, properties);
    }
    _outgoingPhysicsDirtyFlags = 0;
#endif // USE_BULLET_PHYSICS
}
