//
//  ObjectActionSpring.cpp
//  libraries/physics/src
//
//  Created by Seth Alves 2015-6-5
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ObjectMotionState.h"
#include "BulletUtil.h"

#include "ObjectActionSpring.h"

ObjectActionSpring::ObjectActionSpring(QUuid id, EntityItemPointer ownerEntity) :
    ObjectAction(id, ownerEntity) {
    #if WANT_DEBUG
    qDebug() << "ObjectActionSpring::ObjectActionSpring";
    #endif
}

ObjectActionSpring::~ObjectActionSpring() {
    #if WANT_DEBUG
    qDebug() << "ObjectActionSpring::~ObjectActionSpring";
    #endif
}

void ObjectActionSpring::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) {
    if (!tryLockForRead()) {
        // don't risk hanging the thread running the physics simulation
        return;
    }
    void* physicsInfo = _ownerEntity->getPhysicsInfo();

    if (_active && physicsInfo) {
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
        btRigidBody* rigidBody = motionState->getRigidBody();
        if (rigidBody) {
            glm::vec3 offset = _positionalTarget - bulletToGLM(rigidBody->getCenterOfMassPosition());


            // btQuaternion getOrientation() const;
            // const btTransform& getCenterOfMassTransform() const;

            float offsetLength = glm::length(offset);
            float speed = offsetLength; // XXX use _positionalSpringConstant

            float interpolation_value = 0.5; // XXX
            const glm::quat slerped_quat = glm::slerp(bulletToGLM(rigidBody->getOrientation()),
                                                      _rotationalTarget,
                                                      interpolation_value);

            if (offsetLength > IGNORE_POSITION_DELTA) {
                glm::vec3 newVelocity = glm::normalize(offset) * speed;
                rigidBody->setLinearVelocity(glmToBullet(newVelocity));
                // void setAngularVelocity (const btVector3 &ang_vel);
                rigidBody->activate();
            } else {
                rigidBody->setLinearVelocity(glmToBullet(glm::vec3()));
            }
        }
    }
    unlock();
}


bool ObjectActionSpring::updateArguments(QVariantMap arguments) {
    // targets are required, spring-constants are optional
    bool ok = true;
    glm::vec3 positionalTarget =
        EntityActionInterface::extractVec3Argument("spring action", arguments, "positionalTarget", ok);
    bool pscOK = true;
    float positionalSpringConstant =
        EntityActionInterface::extractFloatArgument("spring action", arguments, "positionalSpringConstant", pscOK);

    glm::quat rotationalTarget =
        EntityActionInterface::extractQuatArgument("spring action", arguments, "rotationalTarget", ok);
    bool rscOK = true;
    float rotationalSpringConstant =
        EntityActionInterface::extractFloatArgument("spring action", arguments, "rotationalSpringConstant", rscOK);

    if (!ok) {
        return false;
    }

    lockForWrite();

    _positionalTarget = positionalTarget;
    if (pscOK) {
        _positionalSpringConstant = positionalSpringConstant;
    } else {
        _positionalSpringConstant = 0.5; // XXX pick a good default;
    }

    _rotationalTarget = rotationalTarget;
    if (rscOK) {
        _rotationalSpringConstant = rotationalSpringConstant;
    } else {
        _rotationalSpringConstant = 0.5; // XXX pick a good default;
    }

    _active = true;
    unlock();
    return true;
}
