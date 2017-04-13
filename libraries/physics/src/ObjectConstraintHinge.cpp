//
//  ObjectConstraintHinge.cpp
//  libraries/physics/src
//
//  Created by Seth Alves 2017-4-11
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QVariantGLM.h"

#include "EntityTree.h"
#include "ObjectConstraintHinge.h"
#include "PhysicsLogging.h"


const uint16_t ObjectConstraintHinge::constraintVersion = 1;


ObjectConstraintHinge::ObjectConstraintHinge(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectConstraint(DYNAMIC_TYPE_HINGE, id, ownerEntity),
    _pivotInA(glm::vec3(0.0f)),
    _axisInA(glm::vec3(0.0f))
{
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectConstraintHinge::ObjectConstraintHinge";
    #endif
}

ObjectConstraintHinge::~ObjectConstraintHinge() {
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectConstraintHinge::~ObjectConstraintHinge";
    #endif
}

btTypedConstraint* ObjectConstraintHinge::getConstraint() {
    btHingeConstraint* constraint { nullptr };
    QUuid otherEntityID;
    glm::vec3 pivotInA;
    glm::vec3 axisInA;
    glm::vec3 pivotInB;
    glm::vec3 axisInB;

    withReadLock([&]{
        constraint = static_cast<btHingeConstraint*>(_constraint);
        pivotInA = _pivotInA;
        axisInA = _axisInA;
        otherEntityID = _otherEntityID;
        pivotInB = _pivotInB;
        axisInB = _axisInB;
    });
    if (constraint) {
        return constraint;
    }

    btRigidBody* rigidBodyA = getRigidBody();
    if (!rigidBodyA) {
        qCDebug(physics) << "ObjectConstraintHinge::getConstraint -- no rigidBodyA";
        return nullptr;
    }

    bool useReferenceFrameA { false };

    if (!otherEntityID.isNull()) {
        // This hinge is between two entities... find the other rigid body.
        EntityItemPointer otherEntity = getEntityByID(otherEntityID);
        if (!otherEntity) {
            return nullptr;
        }

        void* otherPhysicsInfo = otherEntity->getPhysicsInfo();
        if (!otherPhysicsInfo) {
            return nullptr;
        }
        ObjectMotionState* otherMotionState = static_cast<ObjectMotionState*>(otherPhysicsInfo);
        if (!otherMotionState) {
            return nullptr;
        }
        btRigidBody* rigidBodyB = otherMotionState->getRigidBody();
        if (!rigidBodyB) {
            return nullptr;
        }

        btTransform rigidBodyAFrame(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f), glmToBullet(pivotInA));
        btTransform rigidBodyBFrame(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f), glmToBullet(pivotInB));

        constraint = new btHingeConstraint(*rigidBodyA, *rigidBodyB,
                                           rigidBodyAFrame, rigidBodyBFrame,
                                           useReferenceFrameA);

        btVector3 bulletAxisInA = glmToBullet(axisInA);
        constraint->setAxis(bulletAxisInA);

        withWriteLock([&]{
            _otherEntity = otherEntity; // save weak-pointer to the other entity
            _constraint = constraint;
        });

        return constraint;
    } else {
        // This hinge is between an entity and the world-frame.
        btTransform rigidBodyAFrame(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f), glmToBullet(pivotInA));

        constraint = new btHingeConstraint(*rigidBodyA, rigidBodyAFrame, useReferenceFrameA);
        // constraint->setAngularOnly(true);

        btVector3 bulletAxisInA = glmToBullet(axisInA);
        constraint->setAxis(bulletAxisInA);

        withWriteLock([&]{
            _otherEntity.reset();
            _constraint = constraint;
        });
        return constraint;
    }
}


bool ObjectConstraintHinge::updateArguments(QVariantMap arguments) {
    glm::vec3 pivotInA;
    glm::vec3 axisInA;
    QUuid otherEntityID;
    glm::vec3 pivotInB;
    glm::vec3 axisInB;

    bool needUpdate = false;
    bool somethingChanged = ObjectDynamic::updateArguments(arguments);
    withReadLock([&]{
        bool ok = true;
        pivotInA = EntityDynamicInterface::extractVec3Argument("hinge constraint", arguments, "pivot", ok, false);
        if (!ok) {
            pivotInA = _pivotInA;
        }

        ok = true;
        axisInA = EntityDynamicInterface::extractVec3Argument("hinge constraint", arguments, "axis", ok, false);
        if (!ok) {
            axisInA = _axisInA;
        }

        ok = true;
        otherEntityID = QUuid(EntityDynamicInterface::extractStringArgument("hinge constraint",
                                                                            arguments, "otherEntityID", ok, false));
        if (!ok) {
            otherEntityID = QUuid();
        }

        ok = true;
        pivotInB = EntityDynamicInterface::extractVec3Argument("hinge constraint", arguments, "otherPivot", ok, false);
        if (!ok) {
            pivotInB = _pivotInB;
        }

        ok = true;
        axisInB = EntityDynamicInterface::extractVec3Argument("hinge constraint", arguments, "otherAxis", ok, false);
        if (!ok) {
            axisInB = _axisInB;
        }

        if (somethingChanged ||
            pivotInA != _pivotInA ||
            axisInA != _axisInA ||
            pivotInB != _pivotInB ||
            axisInB != _axisInB) {
            // something changed
            needUpdate = true;
        }
    });

    if (needUpdate) {
        withWriteLock([&] {
            _pivotInA = pivotInA;
            _axisInA = axisInA;
            _otherEntityID = otherEntityID;
            _pivotInB = pivotInB;
            _axisInB = axisInB;

            _active = true;

            auto ownerEntity = _ownerEntity.lock();
            if (ownerEntity) {
                ownerEntity->setDynamicDataDirty(true);
                ownerEntity->setDynamicDataNeedsTransmit(true);
            }
        });
        // activateBody();
    }

    return true;
}

QVariantMap ObjectConstraintHinge::getArguments() {
    QVariantMap arguments = ObjectDynamic::getArguments();
    withReadLock([&] {
        arguments["pivot"] = glmToQMap(_pivotInA);
        arguments["axis"] = glmToQMap(_axisInA);
        arguments["otherEntityID"] = _otherEntityID;
        arguments["otherPivot"] = glmToQMap(_pivotInB);
        arguments["otherAxis"] = glmToQMap(_axisInB);
    });
    return arguments;
}

QByteArray ObjectConstraintHinge::serialize() const {
    QByteArray serializedConstraintArguments;
    QDataStream dataStream(&serializedConstraintArguments, QIODevice::WriteOnly);

    dataStream << DYNAMIC_TYPE_HINGE;
    dataStream << getID();
    dataStream << ObjectConstraintHinge::constraintVersion;

    withReadLock([&] {
        dataStream << _pivotInA;
        dataStream << _axisInA;
        dataStream << _otherEntityID;
        dataStream << _pivotInB;
        dataStream << _axisInB;
        dataStream << localTimeToServerTime(_expires);
        dataStream << _tag;
    });

    return serializedConstraintArguments;
}

void ObjectConstraintHinge::deserialize(QByteArray serializedArguments) {
    QDataStream dataStream(serializedArguments);

    EntityDynamicType type;
    dataStream >> type;
    assert(type == getType());

    QUuid id;
    dataStream >> id;
    assert(id == getID());

    uint16_t serializationVersion;
    dataStream >> serializationVersion;
    if (serializationVersion != ObjectConstraintHinge::constraintVersion) {
        assert(false);
        return;
    }

    withWriteLock([&] {
        dataStream >> _pivotInA;
        dataStream >> _axisInA;
        dataStream >> _otherEntityID;
        dataStream >> _pivotInB;
        dataStream >> _axisInB;

        quint64 serverExpires;
        dataStream >> serverExpires;
        _expires = serverTimeToLocalTime(serverExpires);

        dataStream >> _tag;

        _active = true;
    });
}
