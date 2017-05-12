//
//  ObjectActionMotor.cpp
//  libraries/physics/src
//
//  Created by Seth Alves 2017-4-30
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QVariantGLM.h"

#include "ObjectActionMotor.h"

#include "PhysicsLogging.h"

const glm::vec3 MOTOR_MAX_SPEED = glm::vec3(PI*10.0f, PI*10.0f, PI*10.0f);
const float MAX_MOTOR_TIMESCALE = 600.0f; // 10 min is a long time

const uint16_t ObjectActionMotor::motorVersion = 1;


ObjectActionMotor::ObjectActionMotor(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectAction(DYNAMIC_TYPE_MOTOR, id, ownerEntity)
{
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectActionMotor::ObjectActionMotor";
    #endif

    qCWarning(physics) << "action type \"motor\" doesn't yet work.";
}

ObjectActionMotor::~ObjectActionMotor() {
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectActionMotor::~ObjectActionMotor";
    #endif
}

void ObjectActionMotor::updateActionWorker(btScalar deltaTimeStep) {
    SpatiallyNestablePointer other = getOther();

    withReadLock([&]{
        auto ownerEntity = _ownerEntity.lock();
        if (!ownerEntity) {
            return;
        }

        void* physicsInfo = ownerEntity->getPhysicsInfo();
        if (!physicsInfo) {
            return;
        }
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
        btRigidBody* rigidBody = motionState->getRigidBody();
        if (!rigidBody) {
            qCDebug(physics) << "ObjectActionMotor::updateActionWorker no rigidBody";
            return;
        }

        if (_angularTimeScale < MAX_MOTOR_TIMESCALE) {

            if (other) {
                glm::vec3 otherAngularVelocity = other->getAngularVelocity();
                rigidBody->setAngularVelocity(glmToBullet(_angularVelocityTarget + otherAngularVelocity));
            } else {
                rigidBody->setAngularVelocity(glmToBullet(_angularVelocityTarget));
            }
        }
    });
}

const float MIN_TIMESCALE = 0.1f;


bool ObjectActionMotor::updateArguments(QVariantMap arguments) {
    glm::vec3 angularVelocityTarget;
    float angularTimeScale;
    QUuid otherID;
    bool ok;


    bool needUpdate = false;
    bool somethingChanged = ObjectDynamic::updateArguments(arguments);
    withReadLock([&]{

        ok = true;
        angularVelocityTarget = EntityDynamicInterface::extractVec3Argument("motor action", arguments,
                                                                            "targetAngularVelocity", ok, false);
        if (!ok) {
            angularVelocityTarget = _angularVelocityTarget;
        }

        ok = true;
        angularTimeScale =
            EntityDynamicInterface::extractFloatArgument("motor action", arguments, "angularTimeScale", ok, false);
        if (!ok) {
            angularTimeScale = _angularTimeScale;
        }

        ok = true;
        otherID = QUuid(EntityDynamicInterface::extractStringArgument("motor action", arguments, "otherID", ok, false));
        if (!ok) {
            otherID = _otherID;
        }

        if (somethingChanged ||
            angularVelocityTarget != _angularVelocityTarget ||
            angularTimeScale != _angularTimeScale ||
            otherID != _otherID) {
            // something changed
            needUpdate = true;
        }
    });

    if (needUpdate) {
        withWriteLock([&] {
            _angularVelocityTarget = angularVelocityTarget;
            _angularTimeScale = glm::max(MIN_TIMESCALE, glm::abs(angularTimeScale));
            _otherID = otherID;
            _active = true;

            auto ownerEntity = _ownerEntity.lock();
            if (ownerEntity) {
                ownerEntity->setDynamicDataDirty(true);
                ownerEntity->setDynamicDataNeedsTransmit(true);
            }
        });
        activateBody();
    }

    return true;
}

QVariantMap ObjectActionMotor::getArguments() {
    QVariantMap arguments = ObjectDynamic::getArguments();
    withReadLock([&] {
        arguments["targetAngularVelocity"] = glmToQMap(_angularVelocityTarget);
        arguments["angularTimeScale"] = _angularTimeScale;

        arguments["otherID"] = _otherID;
    });
    return arguments;
}

void ObjectActionMotor::serializeParameters(QDataStream& dataStream) const {
    withReadLock([&] {
        dataStream << localTimeToServerTime(_expires);
        dataStream << _tag;
        dataStream << _otherID;

        dataStream << _angularVelocityTarget;
        dataStream << _angularTimeScale;
    });
}

QByteArray ObjectActionMotor::serialize() const {
    QByteArray serializedActionArguments;
    QDataStream dataStream(&serializedActionArguments, QIODevice::WriteOnly);

    dataStream << DYNAMIC_TYPE_MOTOR;
    dataStream << getID();
    dataStream << ObjectActionMotor::motorVersion;

    serializeParameters(dataStream);

    return serializedActionArguments;
}

void ObjectActionMotor::deserializeParameters(QByteArray serializedArguments, QDataStream& dataStream) {
    withWriteLock([&] {
        quint64 serverExpires;
        dataStream >> serverExpires;
        _expires = serverTimeToLocalTime(serverExpires);
        dataStream >> _tag;
        dataStream >> _otherID;

        dataStream >> _angularVelocityTarget;
        dataStream >> _angularTimeScale;

        _active = true;
    });
}

void ObjectActionMotor::deserialize(QByteArray serializedArguments) {
    QDataStream dataStream(serializedArguments);

    EntityDynamicType type;
    dataStream >> type;
    assert(type == getType());

    QUuid id;
    dataStream >> id;
    assert(id == getID());

    uint16_t serializationVersion;
    dataStream >> serializationVersion;
    if (serializationVersion != ObjectActionMotor::motorVersion) {
        assert(false);
        return;
    }

    deserializeParameters(serializedArguments, dataStream);
}
