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

QSet<EntityItem*>* _outgoingEntityList;

// static 
void EntityMotionState::setOutgoingEntityList(QSet<EntityItem*>* list) {
    assert(list);
    _outgoingEntityList = list;
}

// static 
void EntityMotionState::enqueueOutgoingEntity(EntityItem* entity) {
    assert(_outgoingEntityList);
    _outgoingEntityList->insert(entity);
}

EntityMotionState::EntityMotionState(EntityItem* entity) 
    :   _entity(entity) {
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
    EntityMotionState::enqueueOutgoingEntity(_entity);
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

void EntityMotionState::sendUpdate(OctreeEditPacketSender* packetSender, uint32_t frame) {
#ifdef USE_BULLET_PHYSICS
    if (_outgoingPacketFlags) {
        EntityItemProperties properties = _entity->getProperties();

        if (_outgoingPacketFlags & EntityItem::DIRTY_POSITION) {
            btTransform worldTrans = _body->getWorldTransform();
            bulletToGLM(worldTrans.getOrigin(), _sentPosition);
            properties.setPosition(_sentPosition + ObjectMotionState::getWorldOffset());
        
            bulletToGLM(worldTrans.getRotation(), _sentRotation);
            properties.setRotation(_sentRotation);
        }
    
        if (_outgoingPacketFlags & EntityItem::DIRTY_VELOCITY) {
            if (_body->isActive()) {
                bulletToGLM(_body->getLinearVelocity(), _sentVelocity);
                bulletToGLM(_body->getAngularVelocity(), _sentAngularVelocity);

                // if the speeds are very small we zero them out
                const float MINIMUM_EXTRAPOLATION_SPEED_SQUARED = 4.0e-6f; // 2mm/sec
                bool zeroSpeed = (glm::length2(_sentVelocity) < MINIMUM_EXTRAPOLATION_SPEED_SQUARED);
                if (zeroSpeed) {
                    _sentVelocity = glm::vec3(0.0f);
                }
                const float MINIMUM_EXTRAPOLATION_SPIN_SQUARED = 0.004f; // ~0.01 rotation/sec
                bool zeroSpin = glm::length2(_sentAngularVelocity) < MINIMUM_EXTRAPOLATION_SPIN_SQUARED;
                if (zeroSpin) {
                    _sentAngularVelocity = glm::vec3(0.0f);
                }

                _sentMoving = ! (zeroSpeed && zeroSpin);
            } else {
                _sentVelocity = _sentAngularVelocity = glm::vec3(0.0f);
                _sentMoving = false;
            }
            properties.setVelocity(_sentVelocity);
            bulletToGLM(_body->getGravity(), _sentAcceleration);
            properties.setGravity(_sentAcceleration);
            properties.setAngularVelocity(_sentAngularVelocity);
        }

        // RELIABLE_SEND_HACK: count number of updates for entities at rest so we can stop sending them after some limit.
        if (_sentMoving) {
            _numNonMovingUpdates = 0;
        } else {
            _numNonMovingUpdates++;
        }
        if (_numNonMovingUpdates <= 1) {
            // we only update lastEdited when we're sending new physics data 
            // (i.e. NOT when we just simulate the positions forward, nore when we resend non-moving data)
            quint64 lastSimulated = _entity->getLastSimulated();
            _entity->setLastEdited(lastSimulated);
            properties.setLastEdited(lastSimulated);
        } else {
            properties.setLastEdited(_entity->getLastEdited());
        }

        EntityItemID id(_entity->getID());
        EntityEditPacketSender* entityPacketSender = static_cast<EntityEditPacketSender*>(packetSender);
        entityPacketSender->queueEditEntityMessage(PacketTypeEntityAddOrEdit, id, properties);

        // The outgoing flags only itemized WHAT to send, not WHETHER to send, hence we always set them
        // to the full set.  These flags may be momentarily cleared by incoming external changes.  
        _outgoingPacketFlags = DIRTY_PHYSICS_FLAGS;
        _sentFrame = frame;
    }
#endif // USE_BULLET_PHYSICS
}
