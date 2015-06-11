//
//  ObjectActionPullToPoint.cpp
//  libraries/physics/src
//
//  Created by Seth Alves 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ObjectActionPullToPoint.h"

ObjectActionPullToPoint::ObjectActionPullToPoint(QUuid id, EntityItemPointer ownerEntity) :
    ObjectAction(id, ownerEntity) {
    #if WANT_DEBUG
    qDebug() << "ObjectActionPullToPoint::ObjectActionPullToPoint";
    #endif
}

ObjectActionPullToPoint::~ObjectActionPullToPoint() {
    #if WANT_DEBUG
    qDebug() << "ObjectActionPullToPoint::~ObjectActionPullToPoint";
    #endif
}

void ObjectActionPullToPoint::updateActionWorker(btScalar deltaTimeStep) {
    if (!tryLockForRead()) {
        // don't risk hanging the thread running the physics simulation
        return;
    }

    void* physicsInfo = _ownerEntity->getPhysicsInfo();
    if (!physicsInfo) {
        unlock();
        return;
    }
    ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
    btRigidBody* rigidBody = motionState->getRigidBody();
    if (!rigidBody) {
        unlock();
        return;
    }

    glm::vec3 offset = _target - bulletToGLM(rigidBody->getCenterOfMassPosition());
    float offsetLength = glm::length(offset);
    if (offsetLength > IGNORE_POSITION_DELTA) {
        glm::vec3 newVelocity = glm::normalize(offset) * _speed;
        rigidBody->setLinearVelocity(glmToBullet(newVelocity));
        rigidBody->activate();
    } else {
        rigidBody->setLinearVelocity(glmToBullet(glm::vec3()));
    }

    unlock();
}


bool ObjectActionPullToPoint::updateArguments(QVariantMap arguments) {
    bool ok = true;
    glm::vec3 target = EntityActionInterface::extractVec3Argument("pull-to-point action", arguments, "target", ok);
    float speed = EntityActionInterface::extractFloatArgument("pull-to-point action", arguments, "speed", ok);
    if (ok) {
        lockForWrite();
        _target = target;
        _speed = speed;
        _active = true;
        unlock();
        return true;
    }
    return false;
}
