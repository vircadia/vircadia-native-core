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

#include "ObjectMotionState.h"
#include "BulletUtil.h"

#include "ObjectActionPullToPoint.h"

ObjectActionPullToPoint::ObjectActionPullToPoint(QUuid id, EntityItemPointer ownerEntity) :
    ObjectAction(id, ownerEntity) {
    qDebug() << "ObjectActionPullToPoint::ObjectActionPullToPoint";
}

ObjectActionPullToPoint::~ObjectActionPullToPoint() {
    qDebug() << "ObjectActionPullToPoint::~ObjectActionPullToPoint";
}

void ObjectActionPullToPoint::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) {
    glm::vec3 offset = _target - _ownerEntity->getPosition();

    if (glm::length(offset) < IGNORE_POSITION_DELTA) {
        offset = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    glm::vec3 newVelocity = glm::normalize(offset) * _speed;

    void* physicsInfo = _ownerEntity->getPhysicsInfo();
    if (physicsInfo) {
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
        btRigidBody* rigidBody = motionState->getRigidBody();
        if (rigidBody) {
            rigidBody->setLinearVelocity(glmToBullet(newVelocity));
            return;
        }
    }

    _ownerEntity->updateVelocity(newVelocity);
}


bool ObjectActionPullToPoint::updateArguments(QVariantMap arguments) {
    bool ok = true;
    glm::vec3 target = EntityActionInterface::extractVec3Argument("pull-to-point action", arguments, "target", ok);
    float speed = EntityActionInterface::extractFloatArgument("pull-to-point action", arguments, "speed", ok);
    if (ok) {
        _target = target;
        _speed = speed;
        return true;
    }
    return false;
}
