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

#include "ObjectConstraintHinge.h"

#include "PhysicsLogging.h"


const uint16_t ObjectConstraintHinge::constraintVersion = 1;


ObjectConstraintHinge::ObjectConstraintHinge(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectConstraint(DYNAMIC_TYPE_HINGE, id, ownerEntity),
    _pivotInA(glm::vec3(0.0f)),
    _axis(glm::vec3(0.0f))
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
    if (_constraint) {
        return _constraint;
    }

    btRigidBody* rigidBodyA = getRigidBody();
    if (!rigidBodyA) {
        qCDebug(physics) << "ObjectConstraintHinge::getConstraint -- no rigidBodyA";
        return nullptr;
    }

    bool useReferenceFrameA { false };

    btTransform rigidBodyAFrame(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f), glmToBullet(_pivotInA));

    btHingeConstraint* constraint = new btHingeConstraint(*rigidBodyA, rigidBodyAFrame, useReferenceFrameA);
    // constraint->setAngularOnly(true);

    btVector3 axisInA = glmToBullet(_axis);
    constraint->setAxis(axisInA);

    _constraint = constraint;
    return constraint;
}


bool ObjectConstraintHinge::updateArguments(QVariantMap arguments) {
    glm::vec3 pivotInA;
    glm::vec3 axis;

    bool needUpdate = false;
    bool somethingChanged = ObjectDynamic::updateArguments(arguments);
    withReadLock([&]{
        bool ok = true;
        pivotInA = EntityDynamicInterface::extractVec3Argument("hinge constraint", arguments, "pivot", ok, false);
        if (!ok) {
            pivotInA = _pivotInA;
        }

        ok = true;
        axis = EntityDynamicInterface::extractVec3Argument("hinge constraint", arguments, "axis", ok, false);
        if (!ok) {
            axis = _axis;
        }

        if (somethingChanged ||
            pivotInA != _pivotInA ||
            axis != _axis) {
            // something changed
            needUpdate = true;
        }
    });

    if (needUpdate) {
        withWriteLock([&] {
            _pivotInA = pivotInA;
            _axis = axis;
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
        arguments["axis"] = glmToQMap(_axis);
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
        dataStream << _axis;
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
        dataStream >> _axis;

        quint64 serverExpires;
        dataStream >> serverExpires;
        _expires = serverTimeToLocalTime(serverExpires);

        dataStream >> _tag;

        _active = true;
    });
}
