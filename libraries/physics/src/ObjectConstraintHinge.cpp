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
    _rbATranslation(glm::vec3(0.0f)),
    _rbARotation(glm::quat())
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
        return nullptr;
    }

    bool useReferenceFrameA { false };

    btTransform rigidBodyAFrame(glmToBullet(_rbARotation), glmToBullet(_rbATranslation));

    _constraint = new btHingeConstraint(*rigidBodyA, rigidBodyAFrame, useReferenceFrameA);
    return _constraint;
}


bool ObjectConstraintHinge::updateArguments(QVariantMap arguments) {
    glm::vec3 rbATranslation;
    glm::quat rbARotation;

    bool needUpdate = false;
    bool somethingChanged = ObjectConstraint::updateArguments(arguments);
    withReadLock([&]{
        // targets are required, hinge-constants are optional
        bool ok = true;
        rbATranslation = EntityDynamicInterface::extractVec3Argument("hinge constraint", arguments, "targetPosition", ok, false);
        if (!ok) {
            rbATranslation = _rbATranslation;
        }

        ok = true;
        rbARotation = EntityDynamicInterface::extractQuatArgument("hinge constraint", arguments, "targetRotation", ok, false);
        if (!ok) {
            rbARotation = _rbARotation;
        }

        if (somethingChanged ||
            rbATranslation != _rbATranslation ||
            rbARotation != _rbARotation) {
            // something changed
            needUpdate = true;
        }
    });

    if (needUpdate) {
        withWriteLock([&] {
            _rbATranslation = rbATranslation;
            _rbARotation = rbARotation;
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

QVariantMap ObjectConstraintHinge::getArguments() {
    QVariantMap arguments = ObjectConstraint::getArguments();
    withReadLock([&] {
        arguments["targetPosition"] = glmToQMap(_rbATranslation);
        arguments["targetRotation"] = glmToQMap(_rbARotation);
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
        dataStream << _rbATranslation;
        dataStream << _rbARotation;
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
        dataStream >> _rbATranslation;
        dataStream >> _rbARotation;

        quint64 serverExpires;
        dataStream >> serverExpires;
        _expires = serverTimeToLocalTime(serverExpires);

        dataStream >> _tag;

        _active = true;
    });
}
