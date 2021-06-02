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

#include "ObjectConstraintConeTwist.h"

#include <LogHandler.h>

#include "QVariantGLM.h"

#include "EntityTree.h"
#include "PhysicsLogging.h"

const uint16_t CONE_TWIST_VERSION_WITH_UNUSED_PAREMETERS = 1;
const uint16_t ObjectConstraintConeTwist::constraintVersion = 2;
const glm::vec3 DEFAULT_CONE_TWIST_AXIS(1.0f, 0.0f, 0.0f);

ObjectConstraintConeTwist::ObjectConstraintConeTwist(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectConstraint(DYNAMIC_TYPE_CONE_TWIST, id, ownerEntity),
    _axisInA(DEFAULT_CONE_TWIST_AXIS),
    _axisInB(DEFAULT_CONE_TWIST_AXIS)
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
        otherEntityID = _otherID;
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

    withReadLock([&]{
        constraint = static_cast<btConeTwistConstraint*>(_constraint);
        swingSpan1 = _swingSpan1;
        swingSpan2 = _swingSpan2;
        twistSpan = _twistSpan;
    });

    if (!constraint) {
        return;
    }

    constraint->setLimit(swingSpan1,
                         swingSpan2,
                         twistSpan);
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
        otherEntityID = _otherID;
        pivotInB = _pivotInB;
        axisInB = _axisInB;
    });
    if (constraint) {
        return constraint;
    }

    static int repeatMessageID = LogHandler::getInstance().newRepeatedMessageID();

    btRigidBody* rigidBodyA = getRigidBody();
    if (!rigidBodyA) {
        HIFI_FCDEBUG_ID(physics(), repeatMessageID, "ObjectConstraintConeTwist::getConstraint -- no rigidBodyA");
        return nullptr;
    }

    if (glm::length(axisInA) < FLT_EPSILON) {
        qCWarning(physics) << "cone-twist axis cannot be a zero vector";
        axisInA = DEFAULT_CONE_TWIST_AXIS;
    } else {
        axisInA = glm::normalize(axisInA);
    }

    if (!otherEntityID.isNull()) {
        // This coneTwist is between two entities... find the other rigid body.

        if (glm::length(axisInB) < FLT_EPSILON) {
            qCWarning(physics) << "cone-twist axis cannot be a zero vector";
            axisInB = DEFAULT_CONE_TWIST_AXIS;
        } else {
            axisInB = glm::normalize(axisInB);
        }

        glm::quat rotA = glm::rotation(DEFAULT_CONE_TWIST_AXIS, axisInA);
        glm::quat rotB = glm::rotation(DEFAULT_CONE_TWIST_AXIS, axisInB);

        btTransform frameInA(glmToBullet(rotA), glmToBullet(pivotInA));
        btTransform frameInB(glmToBullet(rotB), glmToBullet(pivotInB));

        btRigidBody* rigidBodyB = getOtherRigidBody(otherEntityID);
        if (!rigidBodyB) {
            HIFI_FCDEBUG_ID(physics(), repeatMessageID, "ObjectConstraintConeTwist::getConstraint -- no rigidBodyB");
            return nullptr;
        }

        constraint = new btConeTwistConstraint(*rigidBodyA, *rigidBodyB, frameInA, frameInB);
    } else {
        // This coneTwist is between an entity and the world-frame.

        glm::quat rot = glm::rotation(DEFAULT_CONE_TWIST_AXIS, axisInA);

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
            otherEntityID = _otherID;
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

        if (somethingChanged ||
            pivotInA != _pivotInA ||
            axisInA != _axisInA ||
            otherEntityID != _otherID ||
            pivotInB != _pivotInB ||
            axisInB != _axisInB ||
            swingSpan1 != _swingSpan1 ||
            swingSpan2 != _swingSpan2 ||
            twistSpan != _twistSpan) {
            // something changed
            needUpdate = true;
        }
    });

    if (needUpdate) {
        withWriteLock([&] {
            _pivotInA = pivotInA;
            _axisInA = axisInA;
            _otherID = otherEntityID;
            _pivotInB = pivotInB;
            _axisInB = axisInB;
            _swingSpan1 = swingSpan1;
            _swingSpan2 = swingSpan2;
            _twistSpan = twistSpan;

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

/*@jsdoc
 * The <code>"cone-twist"</code> {@link Entities.ActionType|ActionType} connects two entities with a joint that can move 
 * through a cone and can twist.
 * It has arguments in addition to the common {@link Entities.ActionArguments|ActionArguments}:
 *
 * @typedef {object} Entities.ActionArguments-ConeTwist
 * @property {Uuid} otherEntityID=null - The ID of the other entity that is connected to the joint.
 * @property {Vec3} pivot=0,0,0 - The local offset of the joint relative to the entity's position.
 * @property {Vec3} axis=1,0,0 - The axis of the entity that moves through the cone. Must be a non-zero vector.
 * @property {Vec3} otherPivot=0,0,0 - The local offset of the joint relative to the other entity's position.
 * @property {Vec3} otherAxis=1,0,0 - The axis of the other entity that moves through the cone. Must be a non-zero vector.
 * @property {number} swingSpan1=2*Math.PI - The angle through which the joint can move in one axis of the cone, in radians.
 * @property {number} swingSpan2=2*Math.PI - The angle through which the joint can move in the other axis of the cone, in 
 *     radians.
 * @property {number} twistSpan=2*Math.PI - The angle through with the joint can twist, in radians.
 */
QVariantMap ObjectConstraintConeTwist::getArguments() {
    QVariantMap arguments = ObjectDynamic::getArguments();
    withReadLock([&] {
        arguments["pivot"] = vec3ToQMap(_pivotInA);
        arguments["axis"] = vec3ToQMap(_axisInA);
        arguments["otherEntityID"] = _otherID;
        arguments["otherPivot"] = vec3ToQMap(_pivotInB);
        arguments["otherAxis"] = vec3ToQMap(_axisInB);
        arguments["swingSpan1"] = _swingSpan1;
        arguments["swingSpan2"] = _swingSpan2;
        arguments["twistSpan"] = _twistSpan;
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
        dataStream << _otherID;
        dataStream << _pivotInB;
        dataStream << _axisInB;
        dataStream << _swingSpan1;
        dataStream << _swingSpan2;
        dataStream << _twistSpan;
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
    if (serializationVersion > ObjectConstraintConeTwist::constraintVersion) {
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
        dataStream >> _otherID;
        dataStream >> _pivotInB;
        dataStream >> _axisInB;
        dataStream >> _swingSpan1;
        dataStream >> _swingSpan2;
        dataStream >> _twistSpan;
        if (serializationVersion == CONE_TWIST_VERSION_WITH_UNUSED_PAREMETERS) {
            float softness, biasFactor, relaxationFactor;
            dataStream >> softness;
            dataStream >> biasFactor;
            dataStream >> relaxationFactor;
        }

        _active = true;
    });
}
