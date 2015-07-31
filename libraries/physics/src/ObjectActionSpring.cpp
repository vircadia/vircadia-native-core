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

#include "QVariantGLM.h"

#include "ObjectActionSpring.h"

const float SPRING_MAX_SPEED = 10.0f;

const uint16_t ObjectActionSpring::springVersion = 1;


ObjectActionSpring::ObjectActionSpring(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectAction(ACTION_TYPE_SPRING, id, ownerEntity),
    _positionalTarget(glm::vec3(0.0f)),
    _linearTimeScale(FLT_MAX),
    _positionalTargetSet(true),
    _rotationalTarget(glm::quat()),
    _angularTimeScale(FLT_MAX),
    _rotationalTargetSet(true) {
    #if WANT_DEBUG
    qDebug() << "ObjectActionSpring::ObjectActionSpring";
    #endif
}

ObjectActionSpring::~ObjectActionSpring() {
    #if WANT_DEBUG
    qDebug() << "ObjectActionSpring::~ObjectActionSpring";
    #endif
}

void ObjectActionSpring::updateActionWorker(btScalar deltaTimeStep) {
    if (!tryLockForRead()) {
        // don't risk hanging the thread running the physics simulation
        qDebug() << "ObjectActionSpring::updateActionWorker lock failed";
        return;
    }

    auto ownerEntity = _ownerEntity.lock();
    if (!ownerEntity) {
        return;
    }

    void* physicsInfo = ownerEntity->getPhysicsInfo();
    if (!physicsInfo) {
        unlock();
        return;
    }
    ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
    btRigidBody* rigidBody = motionState->getRigidBody();
    if (!rigidBody) {
        unlock();
        qDebug() << "ObjectActionSpring::updateActionWorker no rigidBody";
        return;
    }

    const float MAX_TIMESCALE = 600.0f; // 10 min is a long time
    if (_linearTimeScale < MAX_TIMESCALE) {
        btVector3 offset = rigidBody->getCenterOfMassPosition() - glmToBullet(_positionalTarget);
        float offsetLength = offset.length();
        float speed = (offsetLength > FLT_EPSILON) ? glm::min(offsetLength / _linearTimeScale, SPRING_MAX_SPEED) : 0.0f;

        // this action is aggresively critically damped and defeats the current velocity
        rigidBody->setLinearVelocity((- speed / offsetLength) * offset);
    }

    if (_angularTimeScale < MAX_TIMESCALE) {
        btVector3 targetVelocity(0.0f, 0.0f, 0.0f);

        btQuaternion bodyRotation = rigidBody->getOrientation();
        auto alignmentDot = bodyRotation.dot(glmToBullet(_rotationalTarget));
        const float ALMOST_ONE = 0.99999f;
        if (glm::abs(alignmentDot) < ALMOST_ONE) {
            btQuaternion target = glmToBullet(_rotationalTarget);
            if (alignmentDot < 0.0f) {
                target = -target;
            }
            // if dQ is the incremental rotation that gets an object from Q0 to Q1 then:
            //
            //      Q1 = dQ * Q0 
            //
            // solving for dQ gives:
            //
            //      dQ = Q1 * Q0^
            btQuaternion deltaQ = target * bodyRotation.inverse();
            float angle = deltaQ.getAngle();
            const float MIN_ANGLE = 1.0e-4;
            if (angle > MIN_ANGLE) {
                targetVelocity = (angle / _angularTimeScale) * deltaQ.getAxis();
            }
        }
        // this action is aggresively critically damped and defeats the current velocity
        rigidBody->setAngularVelocity(targetVelocity);
    }
    unlock();
}

const float MIN_TIMESCALE = 0.1f;

bool ObjectActionSpring::updateArguments(QVariantMap arguments) {
    // targets are required, spring-constants are optional
    bool ok = true;
    glm::vec3 positionalTarget =
        EntityActionInterface::extractVec3Argument("spring action", arguments, "targetPosition", ok, false);
    if (!ok) {
        positionalTarget = _positionalTarget;
    }
    ok = true;
    float linearTimeScale =
        EntityActionInterface::extractFloatArgument("spring action", arguments, "linearTimeScale", ok, false);
    if (!ok || linearTimeScale <= 0.0f) {
        linearTimeScale = _linearTimeScale;
    }

    ok = true;
    glm::quat rotationalTarget =
        EntityActionInterface::extractQuatArgument("spring action", arguments, "targetRotation", ok, false);
    if (!ok) {
        rotationalTarget = _rotationalTarget;
    }

    ok = true;
    float angularTimeScale =
        EntityActionInterface::extractFloatArgument("spring action", arguments, "angularTimeScale", ok, false);
    if (!ok) {
        angularTimeScale = _angularTimeScale;
    }

    if (positionalTarget != _positionalTarget
            || linearTimeScale != _linearTimeScale
            || rotationalTarget != _rotationalTarget
            || angularTimeScale != _angularTimeScale) {
        // something changed
        lockForWrite();
        _positionalTarget = positionalTarget;
        _linearTimeScale = glm::max(MIN_TIMESCALE, glm::abs(linearTimeScale));
        _rotationalTarget = rotationalTarget;
        _angularTimeScale = glm::max(MIN_TIMESCALE, glm::abs(angularTimeScale));
        _active = true;
        activateBody();
        unlock();
    }
    return true;
}

QVariantMap ObjectActionSpring::getArguments() {
    QVariantMap arguments;
    lockForRead();

    arguments["linearTimeScale"] = _linearTimeScale;
    arguments["targetPosition"] = glmToQMap(_positionalTarget);

    arguments["targetRotation"] = glmToQMap(_rotationalTarget);
    arguments["angularTimeScale"] = _angularTimeScale;

    unlock();
    return arguments;
}

QByteArray ObjectActionSpring::serialize() const {
    QByteArray serializedActionArguments;
    QDataStream dataStream(&serializedActionArguments, QIODevice::WriteOnly);

    dataStream << ACTION_TYPE_SPRING;
    dataStream << getID();
    dataStream << ObjectActionSpring::springVersion;

    dataStream << _positionalTarget;
    dataStream << _linearTimeScale;
    dataStream << _positionalTargetSet;

    dataStream << _rotationalTarget;
    dataStream << _angularTimeScale;
    dataStream << _rotationalTargetSet;

    return serializedActionArguments;
}

void ObjectActionSpring::deserialize(QByteArray serializedArguments) {
    QDataStream dataStream(serializedArguments);

    EntityActionType type;
    dataStream >> type;
    assert(type == getType());

    QUuid id;
    dataStream >> id;
    assert(id == getID());

    uint16_t serializationVersion;
    dataStream >> serializationVersion;
    if (serializationVersion != ObjectActionSpring::springVersion) {
        return;
    }

    dataStream >> _positionalTarget;
    dataStream >> _linearTimeScale;
    dataStream >> _positionalTargetSet;

    dataStream >> _rotationalTarget;
    dataStream >> _angularTimeScale;
    dataStream >> _rotationalTargetSet;

    _active = true;
}
