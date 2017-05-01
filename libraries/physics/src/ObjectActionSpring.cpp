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

#include "PhysicsLogging.h"

const float SPRING_MAX_SPEED = 10.0f;
const float MAX_SPRING_TIMESCALE = 600.0f; // 10 min is a long time

const uint16_t ObjectActionSpring::springVersion = 1;


ObjectActionSpring::ObjectActionSpring(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectAction(DYNAMIC_TYPE_SPRING, id, ownerEntity),
    _positionalTarget(glm::vec3(0.0f)),
    _desiredPositionalTarget(glm::vec3(0.0f)),
    _linearTimeScale(FLT_MAX),
    _positionalTargetSet(true),
    _rotationalTarget(glm::quat()),
    _desiredRotationalTarget(glm::quat()),
    _angularTimeScale(FLT_MAX),
    _rotationalTargetSet(true) {
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectActionSpring::ObjectActionSpring";
    #endif
}

ObjectActionSpring::~ObjectActionSpring() {
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectActionSpring::~ObjectActionSpring";
    #endif
}

SpatiallyNestablePointer ObjectActionSpring::getOther() {
    SpatiallyNestablePointer other;
    withWriteLock([&]{
        if (_otherID == QUuid()) {
            // no other
            return;
        }
        other = _other.lock();
        if (other && other->getID() == _otherID) {
            // other is already up-to-date
            return;
        }
        if (other) {
            // we have a pointer to other, but it's wrong
            other.reset();
            _other.reset();
        }
        // we have an other-id but no pointer to other cached
        QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
        if (!parentFinder) {
            return;
        }
        EntityItemPointer ownerEntity = _ownerEntity.lock();
        if (!ownerEntity) {
            return;
        }
        bool success;
        _other = parentFinder->find(_otherID, success, ownerEntity->getParentTree());
        if (success) {
            other = _other.lock();
        }
    });
    return other;
}

bool ObjectActionSpring::getTarget(float deltaTimeStep, glm::quat& rotation, glm::vec3& position,
                                   glm::vec3& linearVelocity, glm::vec3& angularVelocity,
                                   float& linearTimeScale, float& angularTimeScale) {
    SpatiallyNestablePointer other = getOther();
    withReadLock([&]{
        linearTimeScale = _linearTimeScale;
        angularTimeScale = _angularTimeScale;

        if (!_otherID.isNull()) {
            if (other) {
                rotation = _desiredRotationalTarget * other->getRotation();
                position = other->getRotation() * _desiredPositionalTarget + other->getPosition();
            } else {
                // we should have an "other" but can't find it, so disable the spring.
                linearTimeScale = FLT_MAX;
                angularTimeScale = FLT_MAX;
            }
        } else {
            rotation = _desiredRotationalTarget;
            position = _desiredPositionalTarget;
        }
        linearVelocity = glm::vec3();
        angularVelocity = glm::vec3();
    });
    return true;
}

bool ObjectActionSpring::prepareForSpringUpdate(btScalar deltaTimeStep) {
    auto ownerEntity = _ownerEntity.lock();
    if (!ownerEntity) {
        return false;
    }

    glm::quat rotation;
    glm::vec3 position;
    glm::vec3 linearVelocity;
    glm::vec3 angularVelocity;

    bool linearValid = false;
    int linearSpringCount = 0;
    bool angularValid = false;
    int angularSpringCount = 0;

    QList<EntityDynamicPointer> springDerivedActions;
    springDerivedActions.append(ownerEntity->getActionsOfType(DYNAMIC_TYPE_SPRING));
    springDerivedActions.append(ownerEntity->getActionsOfType(DYNAMIC_TYPE_FAR_GRAB));
    springDerivedActions.append(ownerEntity->getActionsOfType(DYNAMIC_TYPE_HOLD));

    foreach (EntityDynamicPointer action, springDerivedActions) {
        std::shared_ptr<ObjectActionSpring> springAction = std::static_pointer_cast<ObjectActionSpring>(action);
        glm::quat rotationForAction;
        glm::vec3 positionForAction;
        glm::vec3 linearVelocityForAction;
        glm::vec3 angularVelocityForAction;
        float linearTimeScale;
        float angularTimeScale;
        bool success = springAction->getTarget(deltaTimeStep,
                                               rotationForAction, positionForAction,
                                               linearVelocityForAction, angularVelocityForAction,
                                               linearTimeScale, angularTimeScale);
        if (success) {
            if (angularTimeScale < MAX_SPRING_TIMESCALE) {
                angularValid = true;
                angularSpringCount++;
                angularVelocity += angularVelocityForAction;
                if (springAction.get() == this) {
                    // only use the rotation for this action
                    rotation = rotationForAction;
                }
            }

            if (linearTimeScale < MAX_SPRING_TIMESCALE) {
                linearValid = true;
                linearSpringCount++;
                position += positionForAction;
                linearVelocity += linearVelocityForAction;
            }
        }
    }

    if ((angularValid && angularSpringCount > 0) || (linearValid && linearSpringCount > 0)) {
        withWriteLock([&]{
            if (linearValid && linearSpringCount > 0) {
                position /= linearSpringCount;
                linearVelocity /= linearSpringCount;
                _positionalTarget = position;
                _linearVelocityTarget = linearVelocity;
                _positionalTargetSet = true;
                _active = true;
            }
            if (angularValid && angularSpringCount > 0) {
                angularVelocity /= angularSpringCount;
                _rotationalTarget = rotation;
                _angularVelocityTarget = angularVelocity;
                _rotationalTargetSet = true;
                _active = true;
            }
        });
    }

    return linearValid || angularValid;
}


void ObjectActionSpring::updateActionWorker(btScalar deltaTimeStep) {
    if (!prepareForSpringUpdate(deltaTimeStep)) {
        return;
    }

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
            qCDebug(physics) << "ObjectActionSpring::updateActionWorker no rigidBody";
            return;
        }

        if (_linearTimeScale < MAX_SPRING_TIMESCALE) {
            btVector3 targetVelocity(0.0f, 0.0f, 0.0f);
            btVector3 offset = rigidBody->getCenterOfMassPosition() - glmToBullet(_positionalTarget);
            float offsetLength = offset.length();
            if (offsetLength > FLT_EPSILON) {
                float speed = glm::min(offsetLength / _linearTimeScale, SPRING_MAX_SPEED);
                targetVelocity = (-speed / offsetLength) * offset;
                if (speed > rigidBody->getLinearSleepingThreshold()) {
                    forceBodyNonStatic();
                    rigidBody->activate();
                }
            }
            // this action is aggresively critically damped and defeats the current velocity
            rigidBody->setLinearVelocity(targetVelocity);
        }

        if (_angularTimeScale < MAX_SPRING_TIMESCALE) {
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
                float speed = deltaQ.getAngle() / _angularTimeScale;
                targetVelocity = speed * deltaQ.getAxis();
                if (speed > rigidBody->getAngularSleepingThreshold()) {
                    rigidBody->activate();
                }
            }
            // this action is aggresively critically damped and defeats the current velocity
            rigidBody->setAngularVelocity(targetVelocity);
        }
    });
}

const float MIN_TIMESCALE = 0.1f;


bool ObjectActionSpring::updateArguments(QVariantMap arguments) {
    glm::vec3 positionalTarget;
    float linearTimeScale;
    glm::quat rotationalTarget;
    float angularTimeScale;
    QUuid otherID;


    bool needUpdate = false;
    bool somethingChanged = ObjectDynamic::updateArguments(arguments);
    withReadLock([&]{
        // targets are required, spring-constants are optional
        bool ok = true;
        positionalTarget = EntityDynamicInterface::extractVec3Argument("spring action", arguments, "targetPosition", ok, false);
        if (!ok) {
            positionalTarget = _desiredPositionalTarget;
        }
        ok = true;
        linearTimeScale = EntityDynamicInterface::extractFloatArgument("spring action", arguments, "linearTimeScale", ok, false);
        if (!ok || linearTimeScale <= 0.0f) {
            linearTimeScale = _linearTimeScale;
        }

        ok = true;
        rotationalTarget = EntityDynamicInterface::extractQuatArgument("spring action", arguments, "targetRotation", ok, false);
        if (!ok) {
            rotationalTarget = _desiredRotationalTarget;
        }

        ok = true;
        angularTimeScale =
            EntityDynamicInterface::extractFloatArgument("spring action", arguments, "angularTimeScale", ok, false);
        if (!ok) {
            angularTimeScale = _angularTimeScale;
        }

        ok = true;
        otherID = QUuid(EntityDynamicInterface::extractStringArgument("spring action",
                                                                            arguments, "otherID", ok, false));
        if (!ok) {
            otherID = _otherID;
        }

        if (somethingChanged ||
            positionalTarget != _desiredPositionalTarget ||
            linearTimeScale != _linearTimeScale ||
            rotationalTarget != _desiredRotationalTarget ||
            angularTimeScale != _angularTimeScale ||
            otherID != _otherID) {
            // something changed
            needUpdate = true;
        }
    });

    if (needUpdate) {
        withWriteLock([&] {
            _desiredPositionalTarget = positionalTarget;
            _linearTimeScale = glm::max(MIN_TIMESCALE, glm::abs(linearTimeScale));
            _desiredRotationalTarget = rotationalTarget;
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

QVariantMap ObjectActionSpring::getArguments() {
    QVariantMap arguments = ObjectDynamic::getArguments();
    withReadLock([&] {
        arguments["linearTimeScale"] = _linearTimeScale;
        arguments["targetPosition"] = glmToQMap(_desiredPositionalTarget);

        arguments["targetRotation"] = glmToQMap(_desiredRotationalTarget);
        arguments["angularTimeScale"] = _angularTimeScale;

        arguments["otherID"] = _otherID;
    });
    return arguments;
}

void ObjectActionSpring::serializeParameters(QDataStream& dataStream) const {
    withReadLock([&] {
        dataStream << _desiredPositionalTarget;
        dataStream << _linearTimeScale;
        dataStream << _positionalTargetSet;
        dataStream << _desiredRotationalTarget;
        dataStream << _angularTimeScale;
        dataStream << _rotationalTargetSet;
        dataStream << localTimeToServerTime(_expires);
        dataStream << _tag;
        dataStream << _otherID;
    });
}

QByteArray ObjectActionSpring::serialize() const {
    QByteArray serializedActionArguments;
    QDataStream dataStream(&serializedActionArguments, QIODevice::WriteOnly);

    dataStream << DYNAMIC_TYPE_SPRING;
    dataStream << getID();
    dataStream << ObjectActionSpring::springVersion;

    serializeParameters(dataStream);

    return serializedActionArguments;
}

void ObjectActionSpring::deserializeParameters(QByteArray serializedArguments, QDataStream& dataStream) {
    withWriteLock([&] {
        dataStream >> _desiredPositionalTarget;
        dataStream >> _linearTimeScale;
        dataStream >> _positionalTargetSet;

        dataStream >> _desiredRotationalTarget;
        dataStream >> _angularTimeScale;
        dataStream >> _rotationalTargetSet;

        quint64 serverExpires;
        dataStream >> serverExpires;
        _expires = serverTimeToLocalTime(serverExpires);

        dataStream >> _tag;

        dataStream >> _otherID;

        _active = true;
    });
}

void ObjectActionSpring::deserialize(QByteArray serializedArguments) {
    QDataStream dataStream(serializedArguments);

    EntityDynamicType type;
    dataStream >> type;
    assert(type == getType());

    QUuid id;
    dataStream >> id;
    assert(id == getID());

    uint16_t serializationVersion;
    dataStream >> serializationVersion;
    if (serializationVersion != ObjectActionSpring::springVersion) {
        assert(false);
        return;
    }

    deserializeParameters(serializedArguments, dataStream);
}
