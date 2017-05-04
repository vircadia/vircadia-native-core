//
//  ObjectConstraintSlider.cpp
//  libraries/physics/src
//
//  Created by Seth Alves 2017-4-23
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QVariantGLM.h"

#include "EntityTree.h"
#include "ObjectConstraintSlider.h"
#include "PhysicsLogging.h"


const uint16_t ObjectConstraintSlider::constraintVersion = 1;


ObjectConstraintSlider::ObjectConstraintSlider(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectConstraint(DYNAMIC_TYPE_SLIDER, id, ownerEntity),
    _pointInA(glm::vec3(0.0f)),
    _axisInA(glm::vec3(0.0f))
{
}

ObjectConstraintSlider::~ObjectConstraintSlider() {
}

QList<btRigidBody*> ObjectConstraintSlider::getRigidBodies() {
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

void ObjectConstraintSlider::prepareForPhysicsSimulation() {
}

void ObjectConstraintSlider::updateSlider() {
    btSliderConstraint* constraint { nullptr };

    withReadLock([&]{
        constraint = static_cast<btSliderConstraint*>(_constraint);
    });

    if (!constraint) {
        return;
    }

    // constraint->setFrames (const btTransform &frameA, const btTransform &frameB);
    constraint->setLowerLinLimit(_linearLow);
    constraint->setUpperLinLimit(_linearHigh);
    constraint->setLowerAngLimit(_angularLow);
    constraint->setUpperAngLimit(_angularHigh);

}


btTypedConstraint* ObjectConstraintSlider::getConstraint() {
    btSliderConstraint* constraint { nullptr };
    QUuid otherEntityID;
    glm::vec3 pointInA;
    glm::vec3 axisInA;
    glm::vec3 pointInB;
    glm::vec3 axisInB;

    withReadLock([&]{
        constraint = static_cast<btSliderConstraint*>(_constraint);
        pointInA = _pointInA;
        axisInA = _axisInA;
        otherEntityID = _otherEntityID;
        pointInB = _pointInB;
        axisInB = _axisInB;
    });
    if (constraint) {
        return constraint;
    }

    btRigidBody* rigidBodyA = getRigidBody();
    if (!rigidBodyA) {
        qCDebug(physics) << "ObjectConstraintSlider::getConstraint -- no rigidBodyA";
        return nullptr;
    }

    if (!otherEntityID.isNull()) {
        // This slider is between two entities... find the other rigid body.

        glm::quat rotA = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), glm::normalize(axisInA));
        glm::quat rotB = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), glm::normalize(axisInB));

        btTransform frameInA(glmToBullet(rotA), glmToBullet(pointInA));
        btTransform frameInB(glmToBullet(rotB), glmToBullet(pointInB));

        btRigidBody* rigidBodyB = getOtherRigidBody(otherEntityID);
        if (!rigidBodyB) {
            return nullptr;
        }

        constraint = new btSliderConstraint(*rigidBodyA, *rigidBodyB, frameInA, frameInB, true);
    } else {
        // This slider is between an entity and the world-frame.

        glm::quat rot = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), glm::normalize(axisInA));

        btTransform frameInA(glmToBullet(rot), glmToBullet(pointInA));

        constraint = new btSliderConstraint(*rigidBodyA, frameInA, true);
    }

    withWriteLock([&]{
        _constraint = constraint;
    });

    // if we don't wake up rigidBodyA, we may not send the dynamicData property over the network
    forceBodyNonStatic();
    activateBody();

    updateSlider();

    return constraint;
}


bool ObjectConstraintSlider::updateArguments(QVariantMap arguments) {
    glm::vec3 pointInA;
    glm::vec3 axisInA;
    QUuid otherEntityID;
    glm::vec3 pointInB;
    glm::vec3 axisInB;
    float linearLow;
    float linearHigh;
    float angularLow;
    float angularHigh;

    bool needUpdate = false;
    bool somethingChanged = ObjectDynamic::updateArguments(arguments);
    withReadLock([&]{
        bool ok = true;
        pointInA = EntityDynamicInterface::extractVec3Argument("slider constraint", arguments, "point", ok, false);
        if (!ok) {
            pointInA = _pointInA;
        }

        ok = true;
        axisInA = EntityDynamicInterface::extractVec3Argument("slider constraint", arguments, "axis", ok, false);
        if (!ok) {
            axisInA = _axisInA;
        }

        ok = true;
        otherEntityID = QUuid(EntityDynamicInterface::extractStringArgument("slider constraint",
                                                                            arguments, "otherEntityID", ok, false));
        if (!ok) {
            otherEntityID = _otherEntityID;
        }

        ok = true;
        pointInB = EntityDynamicInterface::extractVec3Argument("slider constraint", arguments, "otherPoint", ok, false);
        if (!ok) {
            pointInB = _pointInB;
        }

        ok = true;
        axisInB = EntityDynamicInterface::extractVec3Argument("slider constraint", arguments, "otherAxis", ok, false);
        if (!ok) {
            axisInB = _axisInB;
        }

        ok = true;
        linearLow = EntityDynamicInterface::extractFloatArgument("slider constraint", arguments, "linearLow", ok, false);
        if (!ok) {
            linearLow = _linearLow;
        }

        ok = true;
        linearHigh = EntityDynamicInterface::extractFloatArgument("slider constraint", arguments, "linearHigh", ok, false);
        if (!ok) {
            linearHigh = _linearHigh;
        }

        ok = true;
        angularLow = EntityDynamicInterface::extractFloatArgument("slider constraint", arguments, "angularLow", ok, false);
        if (!ok) {
            angularLow = _angularLow;
        }

        ok = true;
        angularHigh = EntityDynamicInterface::extractFloatArgument("slider constraint", arguments, "angularHigh", ok, false);
        if (!ok) {
            angularHigh = _angularHigh;
        }

        if (somethingChanged ||
            pointInA != _pointInA ||
            axisInA != _axisInA ||
            otherEntityID != _otherEntityID ||
            pointInB != _pointInB ||
            axisInB != _axisInB ||
            linearLow != _linearLow ||
            linearHigh != _linearHigh ||
            angularLow != _angularLow ||
            angularHigh != _angularHigh) {
            // something changed
            needUpdate = true;
        }
    });

    if (needUpdate) {
        withWriteLock([&] {
            _pointInA = pointInA;
            _axisInA = axisInA;
            _otherEntityID = otherEntityID;
            _pointInB = pointInB;
            _axisInB = axisInB;
            _linearLow = linearLow;
            _linearHigh = linearHigh;
            _angularLow = angularLow;
            _angularHigh = angularHigh;

            _active = true;

            auto ownerEntity = _ownerEntity.lock();
            if (ownerEntity) {
                ownerEntity->setDynamicDataDirty(true);
                ownerEntity->setDynamicDataNeedsTransmit(true);
            }
        });

        updateSlider();
    }

    return true;
}

QVariantMap ObjectConstraintSlider::getArguments() {
    QVariantMap arguments = ObjectDynamic::getArguments();
    withReadLock([&] {
        if (_constraint) {
            arguments["point"] = glmToQMap(_pointInA);
            arguments["axis"] = glmToQMap(_axisInA);
            arguments["otherEntityID"] = _otherEntityID;
            arguments["otherPoint"] = glmToQMap(_pointInB);
            arguments["otherAxis"] = glmToQMap(_axisInB);
            arguments["linearLow"] = _linearLow;
            arguments["linearHigh"] = _linearHigh;
            arguments["angularLow"] = _angularLow;
            arguments["angularHigh"] = _angularHigh;
            arguments["linearPosition"] = static_cast<btSliderConstraint*>(_constraint)->getLinearPos();
            arguments["angularPosition"] = static_cast<btSliderConstraint*>(_constraint)->getAngularPos();
        }
    });
    return arguments;
}

QByteArray ObjectConstraintSlider::serialize() const {
    QByteArray serializedConstraintArguments;
    QDataStream dataStream(&serializedConstraintArguments, QIODevice::WriteOnly);

    dataStream << DYNAMIC_TYPE_SLIDER;
    dataStream << getID();
    dataStream << ObjectConstraintSlider::constraintVersion;

    withReadLock([&] {
        dataStream << localTimeToServerTime(_expires);
        dataStream << _tag;

        dataStream << _pointInA;
        dataStream << _axisInA;
        dataStream << _otherEntityID;
        dataStream << _pointInB;
        dataStream << _axisInB;
        dataStream << _linearLow;
        dataStream << _linearHigh;
        dataStream << _angularLow;
        dataStream << _angularHigh;
    });

    return serializedConstraintArguments;
}

void ObjectConstraintSlider::deserialize(QByteArray serializedArguments) {
    QDataStream dataStream(serializedArguments);

    EntityDynamicType type;
    dataStream >> type;
    assert(type == getType());

    QUuid id;
    dataStream >> id;
    assert(id == getID());

    uint16_t serializationVersion;
    dataStream >> serializationVersion;
    if (serializationVersion != ObjectConstraintSlider::constraintVersion) {
        assert(false);
        return;
    }

    withWriteLock([&] {
        quint64 serverExpires;
        dataStream >> serverExpires;
        _expires = serverTimeToLocalTime(serverExpires);
        dataStream >> _tag;

        dataStream >> _pointInA;
        dataStream >> _axisInA;
        dataStream >> _otherEntityID;
        dataStream >> _pointInB;
        dataStream >> _axisInB;
        dataStream >> _linearLow;
        dataStream >> _linearHigh;
        dataStream >> _angularLow;
        dataStream >> _angularHigh;

        _active = true;
    });
}
