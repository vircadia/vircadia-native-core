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

#include <BulletUtil.h>

#include "EntityItem.h"
#include "EntityMotionState.h"

EntityMotionState::EntityMotionState(EntityItem* entity) : _entity(entity) {
    assert(entity != NULL);
}

EntityMotionState::~EntityMotionState() {
}

// This callback is invoked by the physics simulation in two cases:
// (1) when the RigidBody is first added to the world
//     (irregardless of MotionType: STATIC, DYNAMIC, or KINEMATIC)
// (2) at the beginning of each simulation frame for KINEMATIC RigidBody's --
//     it is an opportunity for outside code to update the position of the object
void EntityMotionState::getWorldTransform (btTransform &worldTrans) const {
    btVector3 pos;
    glmToBullet(_entity->getPosition(), pos);
    btQuaternion rot;
    glmToBullet(_entity->getRotation(), rot);
    worldTrans.setOrigin(pos);
    worldTrans.setRotation(rot);
}

// This callback is invoked by the physics simulation at the end of each simulation frame...
// iff the corresponding RigidBody is DYNAMIC and has moved.
void EntityMotionState::setWorldTransform (const btTransform &worldTrans) {
    glm::vec3 pos;
    bulletToGLM(worldTrans.getOrigin(), pos);
    _entity->setPositionInMeters(pos);
    glm::quat rot;
    bulletToGLM(worldTrans.getRotation(), rot);
    _entity->setRotation(rot);
}

void EntityMotionState::computeShapeInfo(ShapeInfo& info) {
    // HACK: for now we make everything a box.
    glm::vec3 halfExtents = _entity->getDimensionsInMeters();
    btVector3 bulletHalfExtents;
    glmToBullet(halfExtents, bulletHalfExtents);
    info.setBox(bulletHalfExtents);
}

