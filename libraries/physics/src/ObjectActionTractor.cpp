//
//  ObjectActionTractor.cpp
//  libraries/physics/src
//
//  Created by Seth Alves 2015-5-8
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QVariantGLM.h"

#include "ObjectActionTractor.h"

#include "PhysicsLogging.h"

const float TRACTOR_MAX_SPEED = 10.0f;
const float MAX_TRACTOR_TIMESCALE = 600.0f; // 10 min is a long time

const uint16_t ObjectActionTractor::tractorVersion = 1;


ObjectActionTractor::ObjectActionTractor(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectAction(DYNAMIC_TYPE_TRACTOR, id, ownerEntity),
    _positionalTarget(0.0f),
    _desiredPositionalTarget(0.0f),
    _linearTimeScale(FLT_MAX),
    _positionalTargetSet(false),
    _rotationalTarget(),
    _desiredRotationalTarget(),
    _angularTimeScale(FLT_MAX),
    _rotationalTargetSet(true),
    _linearVelocityTarget(0.0f)
{
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectActionTractor::ObjectActionTractor";
    #endif
}

ObjectActionTractor::~ObjectActionTractor() {
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectActionTractor::~ObjectActionTractor";
    #endif
}

bool ObjectActionTractor::getTarget(float deltaTimeStep, glm::quat& rotation, glm::vec3& position,
                                   glm::vec3& linearVelocity, glm::vec3& angularVelocity,
                                   float& linearTimeScale, float& angularTimeScale) {
    SpatiallyNestablePointer other = getOther();
    withReadLock([&]{
        linearTimeScale = _linearTimeScale;
        angularTimeScale = _angularTimeScale;

        if (!_otherID.isNull()) {
            if (other) {
                rotation = _desiredRotationalTarget * other->getWorldOrientation();
                position = other->getWorldOrientation() * _desiredPositionalTarget + other->getWorldPosition();
            } else {
                // we should have an "other" but can't find it, so disable the tractor.
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

bool ObjectActionTractor::prepareForTractorUpdate(btScalar deltaTimeStep) {
    auto ownerEntity = _ownerEntity.lock();
    if (!ownerEntity) {
        return false;
    }

    glm::quat rotation;
    glm::vec3 position;
    glm::vec3 angularVelocity;

    bool linearValid = false;
    int linearTractorCount = 0;
    bool angularValid = false;
    int angularTractorCount = 0;

    QList<EntityDynamicPointer> tractorDerivedActions;
    tractorDerivedActions.append(ownerEntity->getActionsOfType(DYNAMIC_TYPE_TRACTOR));
    tractorDerivedActions.append(ownerEntity->getActionsOfType(DYNAMIC_TYPE_FAR_GRAB));
    tractorDerivedActions.append(ownerEntity->getActionsOfType(DYNAMIC_TYPE_HOLD));

    foreach (EntityDynamicPointer action, tractorDerivedActions) {
        std::shared_ptr<ObjectActionTractor> tractorAction = std::static_pointer_cast<ObjectActionTractor>(action);
        glm::quat rotationForAction;
        glm::vec3 positionForAction;
        glm::vec3 linearVelocityForAction;
        glm::vec3 angularVelocityForAction;
        float linearTimeScale;
        float angularTimeScale;
        bool success = tractorAction->getTarget(deltaTimeStep,
                                               rotationForAction, positionForAction,
                                               linearVelocityForAction, angularVelocityForAction,
                                               linearTimeScale, angularTimeScale);
        if (success) {
            if (angularTimeScale < MAX_TRACTOR_TIMESCALE) {
                angularValid = true;
                angularTractorCount++;
                angularVelocity += angularVelocityForAction;
                if (tractorAction.get() == this) {
                    // only use the rotation for this action
                    rotation = rotationForAction;
                }
            }

            if (linearTimeScale < MAX_TRACTOR_TIMESCALE) {
                linearValid = true;
                linearTractorCount++;
                position += positionForAction;
            }
        }
    }

    if ((angularValid && angularTractorCount > 0) || (linearValid && linearTractorCount > 0)) {
        withWriteLock([&]{
            if (linearValid && linearTractorCount > 0) {
                position /= linearTractorCount;
                if (_positionalTargetSet) {
                    _lastPositionTarget = _positionalTarget;
                    _positionalTarget = position;
                    if (deltaTimeStep > EPSILON) {
                        // blend the new velocity with the old (low-pass filter)
                        glm::vec3 newVelocity = (1.0f / deltaTimeStep) * (_positionalTarget - _lastPositionTarget);
                        const float blend = 0.25f;
                        _linearVelocityTarget = (1.0f - blend) * _linearVelocityTarget + blend * newVelocity;
                    }
                }
                _positionalTargetSet = true;
                _active = true;
            }
            if (angularValid && angularTractorCount > 0) {
                angularVelocity /= angularTractorCount;
                _rotationalTarget = rotation;
                _angularVelocityTarget = angularVelocity;
                _rotationalTargetSet = true;
                _active = true;
            }
        });
    }

    return linearValid || angularValid;
}


void ObjectActionTractor::updateActionWorker(btScalar deltaTimeStep) {
    if (!prepareForTractorUpdate(deltaTimeStep)) {
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
            qCDebug(physics) << "ObjectActionTractor::updateActionWorker no rigidBody";
            return;
        }

        if (_linearTimeScale < MAX_TRACTOR_TIMESCALE) {
            btVector3 offsetVelocity(0.0f, 0.0f, 0.0f);
            btVector3 offset = rigidBody->getCenterOfMassPosition() - glmToBullet(_positionalTarget);
            float offsetLength = offset.length();
            if (offsetLength > FLT_EPSILON) {
                float speed = glm::min(offsetLength / _linearTimeScale, TRACTOR_MAX_SPEED);
                offsetVelocity = (-speed / offsetLength) * offset;
                if (speed > rigidBody->getLinearSleepingThreshold()) {
                    forceBodyNonStatic();
                    rigidBody->activate();
                }
            }
            // this action is aggresively critically damped and defeats the current velocity
            rigidBody->setLinearVelocity(glmToBullet(_linearVelocityTarget) + offsetVelocity);
        }

        if (_angularTimeScale < MAX_TRACTOR_TIMESCALE) {
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


bool ObjectActionTractor::updateArguments(QVariantMap arguments) {
    glm::vec3 positionalTarget;
    float linearTimeScale;
    glm::quat rotationalTarget;
    float angularTimeScale;
    QUuid otherID;


    bool needUpdate = false;
    bool somethingChanged = ObjectDynamic::updateArguments(arguments);
    withReadLock([&]{
        // targets are required, tractor-constants are optional
        bool ok = true;
        positionalTarget = EntityDynamicInterface::extractVec3Argument("tractor action", arguments, "targetPosition", ok, false);
        if (!ok) {
            positionalTarget = _desiredPositionalTarget;
        }
        ok = true;
        linearTimeScale = EntityDynamicInterface::extractFloatArgument("tractor action", arguments, "linearTimeScale", ok, false);
        if (!ok || linearTimeScale <= 0.0f) {
            linearTimeScale = _linearTimeScale;
        }

        ok = true;
        rotationalTarget = EntityDynamicInterface::extractQuatArgument("tractor action", arguments, "targetRotation", ok, false);
        if (!ok) {
            rotationalTarget = _desiredRotationalTarget;
        }

        ok = true;
        angularTimeScale =
            EntityDynamicInterface::extractFloatArgument("tractor action", arguments, "angularTimeScale", ok, false);
        if (!ok) {
            angularTimeScale = _angularTimeScale;
        }

        ok = true;
        otherID = QUuid(EntityDynamicInterface::extractStringArgument("tractor action",
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

QVariantMap ObjectActionTractor::getArguments() {
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

void ObjectActionTractor::serializeParameters(QDataStream& dataStream) const {
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

QByteArray ObjectActionTractor::serialize() const {
    QByteArray serializedActionArguments;
    QDataStream dataStream(&serializedActionArguments, QIODevice::WriteOnly);

    dataStream << DYNAMIC_TYPE_TRACTOR;
    dataStream << getID();
    dataStream << ObjectActionTractor::tractorVersion;

    serializeParameters(dataStream);

    return serializedActionArguments;
}

void ObjectActionTractor::deserializeParameters(QByteArray serializedArguments, QDataStream& dataStream) {
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

void ObjectActionTractor::deserialize(QByteArray serializedArguments) {
    QDataStream dataStream(serializedArguments);

    EntityDynamicType type;
    dataStream >> type;
    assert(type == getType());

    QUuid id;
    dataStream >> id;
    assert(id == getID());

    uint16_t serializationVersion;
    dataStream >> serializationVersion;
    if (serializationVersion != ObjectActionTractor::tractorVersion) {
        assert(false);
        return;
    }

    deserializeParameters(serializedArguments, dataStream);
}
