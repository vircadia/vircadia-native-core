//
//  ObjectActionOffset.cpp
//  libraries/physics/src
//
//  Created by Seth Alves 2015-6-17
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QVariantGLM.h"

#include "ObjectActionOffset.h"

const uint16_t ObjectActionOffset::offsetVersion = 1;

ObjectActionOffset::ObjectActionOffset(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectAction(ACTION_TYPE_OFFSET, id, ownerEntity),
    _pointToOffsetFrom(0.0f),
    _linearDistance(0.0f),
    _linearTimeScale(FLT_MAX),
    _positionalTargetSet(false) {
    #if WANT_DEBUG
    qDebug() << "ObjectActionOffset::ObjectActionOffset";
    #endif
}

ObjectActionOffset::~ObjectActionOffset() {
    #if WANT_DEBUG
    qDebug() << "ObjectActionOffset::~ObjectActionOffset";
    #endif
}

void ObjectActionOffset::updateActionWorker(btScalar deltaTimeStep) {
    if (!tryLockForRead()) {
        // don't risk hanging the thread running the physics simulation
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
        qDebug() << "ObjectActionOffset::updateActionWorker no rigidBody";
        return;
    }

    const float MAX_LINEAR_TIMESCALE = 600.0f;  // 10 minutes is a long time
    if (_positionalTargetSet && _linearTimeScale < MAX_LINEAR_TIMESCALE) {
        glm::vec3 objectPosition = bulletToGLM(rigidBody->getCenterOfMassPosition());
        glm::vec3 springAxis = objectPosition - _pointToOffsetFrom; // from anchor to object
        float distance = glm::length(springAxis);
        if (distance > FLT_EPSILON) {
            springAxis /= distance;  // normalize springAxis

            // compute (critically damped) target velocity of spring relaxation
            glm::vec3 offset = (distance - _linearDistance) * springAxis;
            glm::vec3 targetVelocity = (-1.0f / _linearTimeScale) * offset;

            // compute current velocity and its parallel component
            glm::vec3 currentVelocity = bulletToGLM(rigidBody->getLinearVelocity());
            glm::vec3 parallelVelocity = glm::dot(currentVelocity, springAxis) * springAxis;

            // we blend the parallel component with the spring's target velocity to get the new velocity
            float blend = deltaTimeStep / _linearTimeScale;
            if (blend > 1.0f) {
                blend = 1.0f;
            }
            rigidBody->setLinearVelocity(glmToBullet(currentVelocity + blend * (targetVelocity - parallelVelocity)));
        }
    }

    unlock();
}


bool ObjectActionOffset::updateArguments(QVariantMap arguments) {
    bool ok = true;
    glm::vec3 pointToOffsetFrom =
        EntityActionInterface::extractVec3Argument("offset action", arguments, "pointToOffsetFrom", ok, true);
    if (!ok) {
        pointToOffsetFrom = _pointToOffsetFrom;
    }

    ok = true;
    float linearTimeScale =
        EntityActionInterface::extractFloatArgument("offset action", arguments, "linearTimeScale", ok, false);
    if (!ok) { 
        linearTimeScale = _linearTimeScale;
    }

    ok = true;
    float linearDistance =
        EntityActionInterface::extractFloatArgument("offset action", arguments, "linearDistance", ok, false);
    if (!ok) {
        linearDistance = _linearDistance;
    }

    // only change stuff if something actually changed
    if (_pointToOffsetFrom != pointToOffsetFrom
            || _linearTimeScale != linearTimeScale
            || _linearDistance != linearDistance) {
        lockForWrite();
        _pointToOffsetFrom = pointToOffsetFrom;
        _linearTimeScale = linearTimeScale;
        _linearDistance = linearDistance;
        _positionalTargetSet = true;
        _active = true;
        activateBody();
        unlock();
    }
    return true;
}

QVariantMap ObjectActionOffset::getArguments() {
    QVariantMap arguments;
    lockForRead();
    arguments["pointToOffsetFrom"] = glmToQMap(_pointToOffsetFrom);
    arguments["linearTimeScale"] = _linearTimeScale;
    arguments["linearDistance"] = _linearDistance;
    unlock();
    return arguments;
}

QByteArray ObjectActionOffset::serialize() const {
    QByteArray ba;
    QDataStream dataStream(&ba, QIODevice::WriteOnly);
    dataStream << getType();
    dataStream << getID();
    dataStream << ObjectActionOffset::offsetVersion;

    dataStream << _pointToOffsetFrom;
    dataStream << _linearDistance;
    dataStream << _linearTimeScale;
    dataStream << _positionalTargetSet;

    return ba;
}

void ObjectActionOffset::deserialize(QByteArray serializedArguments) {
    QDataStream dataStream(serializedArguments);

    EntityActionType type;
    dataStream >> type;
    assert(type == getType());

    QUuid id;
    dataStream >> id;
    assert(id == getID());

    uint16_t serializationVersion;
    dataStream >> serializationVersion;
    if (serializationVersion != ObjectActionOffset::offsetVersion) {
        return;
    }

    dataStream >> _pointToOffsetFrom;
    dataStream >> _linearDistance;
    dataStream >> _linearTimeScale;
    dataStream >> _positionalTargetSet;

    _active = true;
}
