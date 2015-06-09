//
//  ObjectAction.cpp
//  libraries/physcis/src
//
//  Created by Seth Alves 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntitySimulation.h"

#include "ObjectAction.h"

ObjectAction::ObjectAction(QUuid id, EntityItemPointer ownerEntity) :
    btActionInterface(),
    _id(id),
    _active(false),
    _ownerEntity(ownerEntity) {
}

ObjectAction::~ObjectAction() {
}

void ObjectAction::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) {
    if (!_active) {
        return;
    }
    if (!_ownerEntity) {
        qDebug() << "ObjectActionPullToPoint::updateAction no owner entity";
        return;
    }
    if (!tryLockForRead()) {
        // don't risk hanging the thread running the physics simulation
        return;
    }
    void* physicsInfo = _ownerEntity->getPhysicsInfo();

    if (physicsInfo) {
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
        btRigidBody* rigidBody = motionState->getRigidBody();
        if (rigidBody) {
            updateActionWorker(collisionWorld, deltaTimeStep, motionState, rigidBody);
        }
    }
    unlock();
}

void ObjectAction::debugDraw(btIDebugDraw* debugDrawer) {
}

void ObjectAction::removeFromSimulation(EntitySimulation* simulation) const {
    simulation->removeAction(_id);
}
