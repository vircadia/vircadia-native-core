//
//  ObjectConstraintConeTwist.cpp
//  libraries/physics/src
//
//  Created by Seth Alves 2017-4-29
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QVariantGLM.h"

#include "EntityTree.h"
#include "ObjectConstraintConeTwist.h"
#include "PhysicsLogging.h"


const uint16_t ObjectConstraintConeTwist::constraintVersion = 1;


ObjectConstraintConeTwist::ObjectConstraintConeTwist(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectConstraint(DYNAMIC_TYPE_CONE_TWIST, id, ownerEntity),
    _pivotInA(glm::vec3(0.0f)),
    _axisInA(glm::vec3(0.0f))
{
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectConstraintConeTwist::ObjectConstraintConeTwist";
    #endif
}

ObjectConstraintConeTwist::~ObjectConstraintConeTwist() {
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectConstraintConeTwist::~ObjectConstraintConeTwist";
    #endif
}

QList<btRigidBody*> ObjectConstraintConeTwist::getRigidBodies() {
    QList<btRigidBody*> result;
    result += getRigidBody();
    QUuid otherEntityID;
    withReadLock([&]{
        otherEntityID = _otherEntityID;
    });
    if (!otherEntityID.isNull()) {
        result += getOtherRigidBody(otherEntityID);
    }
    return result;
}

void ObjectConstraintConeTwist::prepareForPhysicsSimulation() {
}

void ObjectConstraintConeTwist::updateConeTwist() {
    btConeTwistConstraint* constraint { nullptr };
    float swingSpan1;
    float swingSpan2;
    float twistSpan;
    float softness;
    float biasFactor;
    float relaxationFactor;

    withReadLock([&]{
        constraint = static_cast<btConeTwistConstraint*>(_constraint);
        swingSpan1 = _swingSpan1;
        swingSpan2 = _swingSpan2;
        twistSpan = _twistSpan;
        softness = _softness;
        biasFactor = _biasFactor;
        relaxationFactor = _relaxationFactor;
    });

    if (!constraint) {
        return;
    }

    constraint->setLimit(swingSpan1,
                         swingSpan2,
                         twistSpan,
                         softness,
                         biasFactor,
                         relaxationFactor);
}


btTypedConstraint* ObjectConstraintConeTwist::getConstraint() {
    btConeTwistConstraint* constraint { nullptr };
    QUuid otherEntityID;
    glm::vec3 pivotInA;
    glm::vec3 axisInA;
    glm::vec3 pivotInB;
    glm::vec3 axisInB;

    withReadLock([&]{
        constraint = static_cast<btConeTwistConstraint*>(_constraint);
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
        qCDebug(physics) << "ObjectConstraintConeTwist::getConstraint -- no rigidBodyA";
        return nullptr;
    }

    if (!otherEntityID.isNull()) {
        // This coneTwist is between two entities... find the other rigid body.

        glm::quat rotA = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), glm::normalize(axisInA));
        glm::quat rotB = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), glm::normalize(axisInB));

        btTransform frameInA(glmToBullet(rotA), glmToBullet(pivotInA));
        btTransform frameInB(glmToBullet(rotB), glmToBullet(pivotInB));

        btRigidBody* rigidBodyB = getOtherRigidBody(otherEntityID);
        if (!rigidBodyB) {
            return nullptr;
        }

        constraint = new btConeTwistConstraint(*rigidBodyA, *rigidBodyB, frameInA, frameInB);
    } else {
        // This coneTwist is between an entity and the world-frame.

        glm::quat rot = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), glm::normalize(axisInA));

        btTransform frameInA(glmToBullet(rot), glmToBullet(pivotInA));

        constraint = new btConeTwistConstraint(*rigidBodyA, frameInA);
    }

    withWriteLock([&]{
        _constraint = constraint;
    });

    // if we don't wake up rigidBodyA, we may not send the dynamicData property over the network
    forceBodyNonStatic();
    activateBody();

    updateConeTwist();

    return constraint;
}


bool ObjectConstraintConeTwist::updateArguments(QVariantMap arguments) {
    glm::vec3 pivotInA;
    glm::vec3 axisInA;
    QUuid otherEntityID;
    glm::vec3 pivotInB;
    glm::vec3 axisInB;
    float swingSpan1;
    float swingSpan2;
    float twistSpan;
    float softness;
    float biasFactor;
    float relaxationFactor;

    bool needUpdate = false;
    bool somethingChanged = ObjectDynamic::updateArguments(arguments);
    withReadLock([&]{
        bool ok = true;
        pivotInA = EntityDynamicInterface::extractVec3Argument("coneTwist constraint", arguments, "pivot", ok, false);
        if (!ok) {
            pivotInA = _pivotInA;
        }

        ok = true;
        axisInA = EntityDynamicInterface::extractVec3Argument("coneTwist constraint", arguments, "axis", ok, false);
        if (!ok) {
            axisInA = _axisInA;
        }

        ok = true;
        otherEntityID = QUuid(EntityDynamicInterface::extractStringArgument("coneTwist constraint",
                                                                            arguments, "otherEntityID", ok, false));
        if (!ok) {
            otherEntityID = _otherEntityID;
        }

        ok = true;
        pivotInB = EntityDynamicInterface::extractVec3Argument("coneTwist constraint", arguments, "otherPivot", ok, false);
        if (!ok) {
            pivotInB = _pivotInB;
        }

        ok = true;
        axisInB = EntityDynamicInterface::extractVec3Argument("coneTwist constraint", arguments, "otherAxis", ok, false);
        if (!ok) {
            axisInB = _axisInB;
        }

        ok = true;
        swingSpan1 = EntityDynamicInterface::extractFloatArgument("coneTwist constraint", arguments, "swingSpan1", ok, false);
        if (!ok) {
            swingSpan1 = _swingSpan1;
        }

        ok = true;
        swingSpan2 = EntityDynamicInterface::extractFloatArgument("coneTwist constraint", arguments, "swingSpan2", ok, false);
        if (!ok) {
            swingSpan2 = _swingSpan2;
        }

        ok = true;
        twistSpan = EntityDynamicInterface::extractFloatArgument("coneTwist constraint", arguments, "twistSpan", ok, false);
        if (!ok) {
            twistSpan = _twistSpan;
        }

        ok = true;
        softness = EntityDynamicInterface::extractFloatArgument("coneTwist constraint", arguments, "softness", ok, false);
        if (!ok) {
            softness = _softness;
        }

        ok = true;
        biasFactor = EntityDynamicInterface::extractFloatArgument("coneTwist constraint", arguments, "biasFactor", ok, false);
        if (!ok) {
            biasFactor = _biasFactor;
        }

        ok = true;
        relaxationFactor =
            EntityDynamicInterface::extractFloatArgument("coneTwist constraint", arguments, "relaxationFactor", ok, false);
        if (!ok) {
            relaxationFactor = _relaxationFactor;
        }

        if (somethingChanged ||
            pivotInA != _pivotInA ||
            axisInA != _axisInA ||
            otherEntityID != _otherEntityID ||
            pivotInB != _pivotInB ||
            axisInB != _axisInB ||
            swingSpan1 != _swingSpan1 ||
            swingSpan2 != _swingSpan2 ||
            twistSpan != _twistSpan ||
            softness != _softness ||
            biasFactor != _biasFactor ||
            relaxationFactor != _relaxationFactor) {
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
            _swingSpan1 = swingSpan1;
            _swingSpan2 = swingSpan2;
            _twistSpan = twistSpan;
            _softness = softness;
            _biasFactor = biasFactor;
            _relaxationFactor = relaxationFactor;

            _active = true;

            auto ownerEntity = _ownerEntity.lock();
            if (ownerEntity) {
                ownerEntity->setDynamicDataDirty(true);
                ownerEntity->setDynamicDataNeedsTransmit(true);
            }
        });

        updateConeTwist();
    }

    return true;
}

QVariantMap ObjectConstraintConeTwist::getArguments() {
    QVariantMap arguments = ObjectDynamic::getArguments();
    withReadLock([&] {
        if (_constraint) {
            arguments["pivot"] = glmToQMap(_pivotInA);
            arguments["axis"] = glmToQMap(_axisInA);
            arguments["otherEntityID"] = _otherEntityID;
            arguments["otherPivot"] = glmToQMap(_pivotInB);
            arguments["otherAxis"] = glmToQMap(_axisInB);
            arguments["swingSpan1"] = _swingSpan1;
            arguments["swingSpan2"] = _swingSpan2;
            arguments["twistSpan"] = _twistSpan;
            arguments["softness"] = _softness;
            arguments["biasFactor"] = _biasFactor;
            arguments["relaxationFactor"] = _relaxationFactor;
        }
    });
    return arguments;
}

QByteArray ObjectConstraintConeTwist::serialize() const {
    QByteArray serializedConstraintArguments;
    QDataStream dataStream(&serializedConstraintArguments, QIODevice::WriteOnly);

    dataStream << DYNAMIC_TYPE_CONE_TWIST;
    dataStream << getID();
    dataStream << ObjectConstraintConeTwist::constraintVersion;

    withReadLock([&] {
        dataStream << localTimeToServerTime(_expires);
        dataStream << _tag;

        dataStream << _pivotInA;
        dataStream << _axisInA;
        dataStream << _otherEntityID;
        dataStream << _pivotInB;
        dataStream << _axisInB;
        dataStream << _swingSpan1;
        dataStream << _swingSpan2;
        dataStream << _twistSpan;
        dataStream << _softness;
        dataStream << _biasFactor;
        dataStream << _relaxationFactor;
    });

    return serializedConstraintArguments;
}

void ObjectConstraintConeTwist::deserialize(QByteArray serializedArguments) {
    QDataStream dataStream(serializedArguments);

    EntityDynamicType type;
    dataStream >> type;
    assert(type == getType());

    QUuid id;
    dataStream >> id;
    assert(id == getID());

    uint16_t serializationVersion;
    dataStream >> serializationVersion;
    if (serializationVersion != ObjectConstraintConeTwist::constraintVersion) {
        assert(false);
        return;
    }

    withWriteLock([&] {
        quint64 serverExpires;
        dataStream >> serverExpires;
        _expires = serverTimeToLocalTime(serverExpires);
        dataStream >> _tag;

        dataStream >> _pivotInA;
        dataStream >> _axisInA;
        dataStream >> _otherEntityID;
        dataStream >> _pivotInB;
        dataStream >> _axisInB;
        dataStream >> _swingSpan1;
        dataStream >> _swingSpan2;
        dataStream >> _twistSpan;
        dataStream >> _softness;
        dataStream >> _biasFactor;
        dataStream >> _relaxationFactor;

        _active = true;
    });
}
