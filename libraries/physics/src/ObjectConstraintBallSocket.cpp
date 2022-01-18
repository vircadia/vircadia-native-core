//
//  ObjectConstraintBallSocket.cpp
//  libraries/physics/src
//
//  Created by Seth Alves 2017-4-29
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ObjectConstraintBallSocket.h"

#include <LogHandler.h>

#include "QVariantGLM.h"

#include "EntityTree.h"
#include "PhysicsLogging.h"


const uint16_t ObjectConstraintBallSocket::constraintVersion = 1;


ObjectConstraintBallSocket::ObjectConstraintBallSocket(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectConstraint(DYNAMIC_TYPE_BALL_SOCKET, id, ownerEntity),
    _pivotInA(glm::vec3(0.0f)),
    _pivotInB(glm::vec3(0.0f))
{
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectConstraintBallSocket::ObjectConstraintBallSocket";
    #endif
}

ObjectConstraintBallSocket::~ObjectConstraintBallSocket() {
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectConstraintBallSocket::~ObjectConstraintBallSocket";
    #endif
}

QList<btRigidBody*> ObjectConstraintBallSocket::getRigidBodies() {
    QList<btRigidBody*> result;
    result += getRigidBody();
    QUuid otherEntityID;
    withReadLock([&]{
        otherEntityID = _otherID;
    });
    if (!otherEntityID.isNull()) {
        result += getOtherRigidBody(otherEntityID);
    }
    return result;
}

void ObjectConstraintBallSocket::prepareForPhysicsSimulation() {
}

void ObjectConstraintBallSocket::updateBallSocket() {
    btPoint2PointConstraint* constraint { nullptr };

    withReadLock([&]{
        constraint = static_cast<btPoint2PointConstraint*>(_constraint);
    });

    if (!constraint) {
        return;
    }

    constraint->setPivotA(glmToBullet(_pivotInA));
    constraint->setPivotB(glmToBullet(_pivotInB));
}


btTypedConstraint* ObjectConstraintBallSocket::getConstraint() {
    btPoint2PointConstraint* constraint { nullptr };
    QUuid otherEntityID;
    glm::vec3 pivotInA;
    glm::vec3 pivotInB;

    withReadLock([&]{
        constraint = static_cast<btPoint2PointConstraint*>(_constraint);
        pivotInA = _pivotInA;
        otherEntityID = _otherID;
        pivotInB = _pivotInB;
    });
    if (constraint) {
        return constraint;
    }

    static int repeatMessageID = LogHandler::getInstance().newRepeatedMessageID();

    btRigidBody* rigidBodyA = getRigidBody();
    if (!rigidBodyA) {
        HIFI_FCDEBUG_ID(physics(), repeatMessageID, "ObjectConstraintBallSocket::getConstraint -- no rigidBodyA");
        return nullptr;
    }

    if (!otherEntityID.isNull()) {
        // This constraint is between two entities... find the other rigid body.

        btRigidBody* rigidBodyB = getOtherRigidBody(otherEntityID);
        if (!rigidBodyB) {
            HIFI_FCDEBUG_ID(physics(), repeatMessageID, "ObjectConstraintBallSocket::getConstraint -- no rigidBodyB");
            return nullptr;
        }

        constraint = new btPoint2PointConstraint(*rigidBodyA, *rigidBodyB, glmToBullet(pivotInA), glmToBullet(pivotInB));
    } else {
        // This constraint is between an entity and the world-frame.

        constraint = new btPoint2PointConstraint(*rigidBodyA, glmToBullet(pivotInA));
    }

    withWriteLock([&]{
        _constraint = constraint;
    });

    // if we don't wake up rigidBodyA, we may not send the dynamicData property over the network
    forceBodyNonStatic();
    activateBody();

    updateBallSocket();

    return constraint;
}


bool ObjectConstraintBallSocket::updateArguments(QVariantMap arguments) {
    glm::vec3 pivotInA;
    QUuid otherEntityID;
    glm::vec3 pivotInB;

    bool needUpdate = false;
    bool somethingChanged = ObjectDynamic::updateArguments(arguments);
    withReadLock([&]{
        bool ok = true;
        pivotInA = EntityDynamicInterface::extractVec3Argument("ball-socket constraint", arguments, "pivot", ok, false);
        if (!ok) {
            pivotInA = _pivotInA;
        }

        ok = true;
        otherEntityID = QUuid(EntityDynamicInterface::extractStringArgument("ball-socket constraint",
                                                                            arguments, "otherEntityID", ok, false));
        if (!ok) {
            otherEntityID = _otherID;
        }

        ok = true;
        pivotInB = EntityDynamicInterface::extractVec3Argument("ball-socket constraint", arguments, "otherPivot", ok, false);
        if (!ok) {
            pivotInB = _pivotInB;
        }

        if (somethingChanged ||
            pivotInA != _pivotInA ||
            otherEntityID != _otherID ||
            pivotInB != _pivotInB) {
            // something changed
            needUpdate = true;
        }
    });

    if (needUpdate) {
        withWriteLock([&] {
            _pivotInA = pivotInA;
            _otherID = otherEntityID;
            _pivotInB = pivotInB;

            _active = true;

            auto ownerEntity = _ownerEntity.lock();
            if (ownerEntity) {
                ownerEntity->setDynamicDataDirty(true);
                ownerEntity->setDynamicDataNeedsTransmit(true);
            }
        });

        updateBallSocket();
    }

    return true;
}

/*@jsdoc
 * The <code>"ball-socket"</code> {@link Entities.ActionType|ActionType} connects two entities with a ball and socket joint. 
 * It has arguments in addition to the common {@link Entities.ActionArguments|ActionArguments}:
 *
 * @typedef {object} Entities.ActionArguments-BallSocket
 * @property {Uuid} otherEntityID=null - The ID of the other entity that is connected to the joint.
 * @property {Vec3} pivot=0,0,0 - The local offset of the joint relative to the entity's position.
 * @property {Vec3} otherPivot=0,0,0 - The local offset of the joint relative to the other entity's position.
 */
QVariantMap ObjectConstraintBallSocket::getArguments() {
    QVariantMap arguments = ObjectDynamic::getArguments();
    withReadLock([&] {
        arguments["pivot"] = vec3ToQMap(_pivotInA);
        arguments["otherEntityID"] = _otherID;
        arguments["otherPivot"] = vec3ToQMap(_pivotInB);
    });
    return arguments;
}

QByteArray ObjectConstraintBallSocket::serialize() const {
    QByteArray serializedConstraintArguments;
    QDataStream dataStream(&serializedConstraintArguments, QIODevice::WriteOnly);

    dataStream << DYNAMIC_TYPE_BALL_SOCKET;
    dataStream << getID();
    dataStream << ObjectConstraintBallSocket::constraintVersion;

    withReadLock([&] {
        dataStream << localTimeToServerTime(_expires);
        dataStream << _tag;

        dataStream << _pivotInA;
        dataStream << _otherID;
        dataStream << _pivotInB;
    });

    return serializedConstraintArguments;
}

void ObjectConstraintBallSocket::deserialize(QByteArray serializedArguments) {
    QDataStream dataStream(serializedArguments);

    EntityDynamicType type;
    dataStream >> type;
    assert(type == getType());

    QUuid id;
    dataStream >> id;
    assert(id == getID());

    uint16_t serializationVersion;
    dataStream >> serializationVersion;
    if (serializationVersion != ObjectConstraintBallSocket::constraintVersion) {
        assert(false);
        return;
    }

    withWriteLock([&] {
        quint64 serverExpires;
        dataStream >> serverExpires;
        _expires = serverTimeToLocalTime(serverExpires);
        dataStream >> _tag;

        dataStream >> _pivotInA;
        dataStream >> _otherID;
        dataStream >> _pivotInB;

        _active = true;
    });
}
