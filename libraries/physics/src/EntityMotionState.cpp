//
//  EntityMotionState.cpp
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2014.11.06
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/norm.hpp>

#include <EntityItem.h>
#include <EntityItemProperties.h>
#include <EntityEditPacketSender.h>
#include <PhysicsCollisionGroups.h>
#include <LogHandler.h>

#include "BulletUtil.h"
#include "EntityMotionState.h"
#include "PhysicsEngine.h"
#include "PhysicsHelpers.h"
#include "PhysicsLogging.h"

#ifdef WANT_DEBUG_ENTITY_TREE_LOCKS
#include "EntityTree.h"
#endif

const uint8_t LOOPS_FOR_SIMULATION_ORPHAN = 50;
const quint64 USECS_BETWEEN_OWNERSHIP_BIDS = USECS_PER_SECOND / 5;

#ifdef WANT_DEBUG_ENTITY_TREE_LOCKS
bool EntityMotionState::entityTreeIsLocked() const {
    EntityTreeElementPointer element = _entity->getElement();
    EntityTreePointer tree = element ? element->getTree() : nullptr;
    if (!tree) {
        return true;
    }
    return true;
}
#else
bool entityTreeIsLocked() {
    return true;
}
#endif


EntityMotionState::EntityMotionState(btCollisionShape* shape, EntityItemPointer entity) :
    ObjectMotionState(nullptr),
    _entityPtr(entity),
    _entity(entity.get()),
    _serverPosition(0.0f),
    _serverRotation(),
    _serverVelocity(0.0f),
    _serverAngularVelocity(0.0f),
    _serverGravity(0.0f),
    _serverAcceleration(0.0f),
    _serverActionData(QByteArray()),
    _lastVelocity(0.0f),
    _measuredAcceleration(0.0f),
    _nextOwnershipBid(0),
    _measuredDeltaTime(0.0f),
    _lastMeasureStep(0),
    _lastStep(0),
    _loopsWithoutOwner(0),
    _accelerationNearlyGravityCount(0),
    _numInactiveUpdates(1)
{
    _type = MOTIONSTATE_TYPE_ENTITY;
    assert(_entity);
    assert(entityTreeIsLocked());
    setMass(_entity->computeMass());
    // we need the side-effects of EntityMotionState::setShape() so we call it explicitly here
    // rather than pass the legit shape pointer to the ObjectMotionState ctor above.
    setShape(shape);

    _outgoingPriority = _entity->getPendingOwnershipPriority();
}

EntityMotionState::~EntityMotionState() {
    assert(_entity);
    _entity = nullptr;
}

void EntityMotionState::updateServerPhysicsVariables() {
    assert(entityTreeIsLocked());
    if (isLocallyOwned()) {
        // don't slam these values if we are the simulation owner
        return;
    }

    Transform localTransform;
    _entity->getLocalTransformAndVelocities(localTransform, _serverVelocity, _serverAngularVelocity);
    _serverVariablesSet = true;
    _serverPosition = localTransform.getTranslation();
    _serverRotation = localTransform.getRotation();
    _serverAcceleration = _entity->getAcceleration();
    _serverActionData = _entity->getDynamicData();
}

void EntityMotionState::handleDeactivation() {
    if (_serverVariablesSet) {
        // copy _server data to entity
        Transform localTransform = _entity->getLocalTransform();
        localTransform.setTranslation(_serverPosition);
        localTransform.setRotation(_serverRotation);
        _entity->setLocalTransformAndVelocities(localTransform, ENTITY_ITEM_ZERO_VEC3, ENTITY_ITEM_ZERO_VEC3);
        // and also to RigidBody
        btTransform worldTrans;
        worldTrans.setOrigin(glmToBullet(_entity->getPosition()));
        worldTrans.setRotation(glmToBullet(_entity->getRotation()));
        _body->setWorldTransform(worldTrans);
        // no need to update velocities... should already be zero
    }
}

// virtual
void EntityMotionState::handleEasyChanges(uint32_t& flags) {
    assert(_entity);
    assert(entityTreeIsLocked());
    updateServerPhysicsVariables();
    ObjectMotionState::handleEasyChanges(flags);

    if (flags & Simulation::DIRTY_SIMULATOR_ID) {
        if (_entity->getSimulatorID().isNull()) {
            // simulation ownership has been removed by an external simulator
            if (glm::length2(_entity->getVelocity()) == 0.0f) {
                // this object is coming to rest --> clear the ACTIVATION flag and _outgoingPriority
                flags &= ~Simulation::DIRTY_PHYSICS_ACTIVATION;
                _body->setActivationState(WANTS_DEACTIVATION);
                _outgoingPriority = 0;
                const float ACTIVATION_EXPIRY = 3.0f; // something larger than the 2.0 hard coded in Bullet
                _body->setDeactivationTime(ACTIVATION_EXPIRY);
            } else {
                // disowned object is still moving --> start timer for ownership bid
                // TODO? put a delay in here proportional to distance from object?
                upgradeOutgoingPriority(VOLUNTEER_SIMULATION_PRIORITY);
                _nextOwnershipBid = usecTimestampNow() + USECS_BETWEEN_OWNERSHIP_BIDS;
            }
            _loopsWithoutOwner = 0;
            _numInactiveUpdates = 0;
        } else if (isLocallyOwned()) {
            // we just inherited ownership, make sure our desired priority matches what we have
            upgradeOutgoingPriority(_entity->getSimulationPriority());
        } else {
            _outgoingPriority = 0;
            _nextOwnershipBid = usecTimestampNow() + USECS_BETWEEN_OWNERSHIP_BIDS;
            _numInactiveUpdates = 0;
        }
    }
    if (flags & Simulation::DIRTY_SIMULATION_OWNERSHIP_PRIORITY) {
        // The DIRTY_SIMULATOR_OWNERSHIP_PRIORITY bit means one of the following:
        // (1) we own it but may need to change the priority OR...
        // (2) we don't own it but should bid (because a local script has been changing physics properties)
        uint8_t newPriority = isLocallyOwned() ? _entity->getSimulationOwner().getPriority() : _entity->getSimulationOwner().getPendingPriority();
        _outgoingPriority = glm::max(_outgoingPriority, newPriority);

        // reset bid expiry so that we bid ASAP
        _nextOwnershipBid = 0;
    }
    if ((flags & Simulation::DIRTY_PHYSICS_ACTIVATION) && !_body->isActive()) {
        if (_body->isKinematicObject()) {
            // only force activate kinematic bodies (dynamic shouldn't need force and
            // active static bodies are special (see PhysicsEngine::_activeStaticBodies))
            _body->activate(true);
            _lastKinematicStep = ObjectMotionState::getWorldSimulationStep();
        } else {
            _body->activate();
        }
    }
}


// virtual
bool EntityMotionState::handleHardAndEasyChanges(uint32_t& flags, PhysicsEngine* engine) {
    assert(_entity);
    updateServerPhysicsVariables();
    return ObjectMotionState::handleHardAndEasyChanges(flags, engine);
}

PhysicsMotionType EntityMotionState::computePhysicsMotionType() const {
    if (!_entity) {
        return MOTION_TYPE_STATIC;
    }
    assert(entityTreeIsLocked());

    if (_entity->getShapeType() == SHAPE_TYPE_STATIC_MESH
        || (_body && _body->getCollisionShape()->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE)) {
        return MOTION_TYPE_STATIC;
    }

    if (_entity->getLocked()) {
        if (_entity->isMoving()) {
            return MOTION_TYPE_KINEMATIC;
        }
        return MOTION_TYPE_STATIC;
    }

    if (_entity->getDynamic()) {
        if (!_entity->getParentID().isNull()) {
            // if something would have been dynamic but is a child of something else, force it to be kinematic, instead.
            return MOTION_TYPE_KINEMATIC;
        }
        return MOTION_TYPE_DYNAMIC;
    }
    if (_entity->isMovingRelativeToParent() ||
        _entity->hasActions() ||
        _entity->hasAncestorOfType(NestableType::Avatar)) {
        return MOTION_TYPE_KINEMATIC;
    }
    return MOTION_TYPE_STATIC;
}

bool EntityMotionState::isMoving() const {
    assert(entityTreeIsLocked());
    return _entity && _entity->isMovingRelativeToParent();
}

// This callback is invoked by the physics simulation in two cases:
// (1) when the RigidBody is first added to the world
//     (irregardless of PhysicsMotionType: STATIC, DYNAMIC, or KINEMATIC)
// (2) at the beginning of each simulation step for KINEMATIC RigidBody's --
//     it is an opportunity for outside code to update the object's simulation position
void EntityMotionState::getWorldTransform(btTransform& worldTrans) const {
    BT_PROFILE("getWorldTransform");
    if (!_entity) {
        return;
    }
    assert(entityTreeIsLocked());
    if (_motionType == MOTION_TYPE_KINEMATIC && !_entity->hasAncestorOfType(NestableType::Avatar)) {
        BT_PROFILE("kinematicIntegration");
        // This is physical kinematic motion which steps strictly by the subframe count
        // of the physics simulation and uses full gravity for acceleration.
        _entity->setAcceleration(_entity->getGravity());

        uint32_t thisStep = ObjectMotionState::getWorldSimulationStep();
        float dt = (thisStep - _lastKinematicStep) * PHYSICS_ENGINE_FIXED_SUBSTEP;
        _entity->stepKinematicMotion(dt);

        // bypass const-ness so we can remember the step
        const_cast<EntityMotionState*>(this)->_lastKinematicStep = thisStep;

        // and set the acceleration-matches-gravity count high so that if we send an update
        // it will use the correct acceleration for remote simulations
        _accelerationNearlyGravityCount = (uint8_t)(-1);
    }
    worldTrans.setOrigin(glmToBullet(getObjectPosition()));
    worldTrans.setRotation(glmToBullet(_entity->getRotation()));
}

// This callback is invoked by the physics simulation at the end of each simulation step...
// iff the corresponding RigidBody is DYNAMIC and ACTIVE.
void EntityMotionState::setWorldTransform(const btTransform& worldTrans) {
    assert(_entity);
    assert(entityTreeIsLocked());
    measureBodyAcceleration();
    bool positionSuccess;
    _entity->setPosition(bulletToGLM(worldTrans.getOrigin()) + ObjectMotionState::getWorldOffset(), positionSuccess, false);
    if (!positionSuccess) {
        static QString repeatedMessage =
            LogHandler::getInstance().addRepeatedMessageRegex("EntityMotionState::setWorldTransform "
                                                              "setPosition failed.*");
        qCDebug(physics) << "EntityMotionState::setWorldTransform setPosition failed" << _entity->getID();
    }
    bool orientationSuccess;
    _entity->setOrientation(bulletToGLM(worldTrans.getRotation()), orientationSuccess, false);
    if (!orientationSuccess) {
        static QString repeatedMessage =
            LogHandler::getInstance().addRepeatedMessageRegex("EntityMotionState::setWorldTransform "
                                                              "setOrientation failed.*");
        qCDebug(physics) << "EntityMotionState::setWorldTransform setOrientation failed" << _entity->getID();
    }
    _entity->setVelocity(getBodyLinearVelocity());
    _entity->setAngularVelocity(getBodyAngularVelocity());
    _entity->setLastSimulated(usecTimestampNow());

    if (_entity->getSimulatorID().isNull()) {
        _loopsWithoutOwner++;
        if (_loopsWithoutOwner > LOOPS_FOR_SIMULATION_ORPHAN && usecTimestampNow() > _nextOwnershipBid) {
            upgradeOutgoingPriority(VOLUNTEER_SIMULATION_PRIORITY);
        }
    }

    #ifdef WANT_DEBUG
        quint64 now = usecTimestampNow();
        qCDebug(physics) << "EntityMotionState::setWorldTransform()... changed entity:" << _entity->getEntityItemID();
        qCDebug(physics) << "       last edited:" << _entity->getLastEdited()
                         << formatUsecTime(now - _entity->getLastEdited()) << "ago";
        qCDebug(physics) << "    last simulated:" << _entity->getLastSimulated()
                         << formatUsecTime(now - _entity->getLastSimulated()) << "ago";
        qCDebug(physics) << "      last updated:" << _entity->getLastUpdated()
                         << formatUsecTime(now - _entity->getLastUpdated()) << "ago";
    #endif
}


// virtual and protected
bool EntityMotionState::isReadyToComputeShape() const {
    return _entity->isReadyToComputeShape();
}

// virtual and protected
const btCollisionShape* EntityMotionState::computeNewShape() {
    ShapeInfo shapeInfo;
    assert(entityTreeIsLocked());
    _entity->computeShapeInfo(shapeInfo);
    return getShapeManager()->getShape(shapeInfo);
}

void EntityMotionState::setShape(const btCollisionShape* shape) {
    if (_shape != shape) {
        ObjectMotionState::setShape(shape);
        _entity->setCollisionShape(_shape);
    }
}

bool EntityMotionState::isCandidateForOwnership() const {
    assert(_body);
    assert(_entity);
    assert(entityTreeIsLocked());
    return _outgoingPriority != 0
        || isLocallyOwned()
        || _entity->dynamicDataNeedsTransmit();
}

bool EntityMotionState::remoteSimulationOutOfSync(uint32_t simulationStep) {
    // NOTE: we only get here if we think we own the simulation
    assert(_body);

    bool parentTransformSuccess;
    Transform localToWorld = _entity->getParentTransform(parentTransformSuccess);
    Transform worldToLocal;
    Transform worldVelocityToLocal;
    if (parentTransformSuccess) {
        localToWorld.evalInverse(worldToLocal);
        worldVelocityToLocal = worldToLocal;
        worldVelocityToLocal.setTranslation(glm::vec3(0.0f));
    }

    // if we've never checked before, our _lastStep will be 0, and we need to initialize our state
    if (_lastStep == 0) {
        btTransform xform = _body->getWorldTransform();
        _serverVariablesSet = true;
        _serverPosition = worldToLocal.transform(bulletToGLM(xform.getOrigin()));
        _serverRotation = worldToLocal.getRotation() * bulletToGLM(xform.getRotation());
        _serverVelocity = worldVelocityToLocal.transform(getBodyLinearVelocityGTSigma());
        _serverAcceleration = Vectors::ZERO;
        _serverAngularVelocity = worldVelocityToLocal.transform(bulletToGLM(_body->getAngularVelocity()));
        _lastStep = simulationStep;
        _serverActionData = _entity->getDynamicData();
        _numInactiveUpdates = 1;
        return false;
    }

    #ifdef WANT_DEBUG
    glm::vec3 wasPosition = _serverPosition;
    glm::quat wasRotation = _serverRotation;
    glm::vec3 wasAngularVelocity = _serverAngularVelocity;
    #endif

    int numSteps = simulationStep - _lastStep;
    float dt = (float)(numSteps) * PHYSICS_ENGINE_FIXED_SUBSTEP;

    if (_numInactiveUpdates > 0) {
        const uint8_t MAX_NUM_INACTIVE_UPDATES = 20;
        if (_numInactiveUpdates > MAX_NUM_INACTIVE_UPDATES) {
            // clear local ownership (stop sending updates) and let the server clear itself
            _entity->clearSimulationOwnership();
            return false;
        }
        // we resend the inactive update every INACTIVE_UPDATE_PERIOD
        // until it is removed from the outgoing updates
        // (which happens when we don't own the simulation and it isn't touching our simulation)
        const float INACTIVE_UPDATE_PERIOD = 0.5f;
        return (dt > INACTIVE_UPDATE_PERIOD * (float)_numInactiveUpdates);
    }

    if (!_body->isActive()) {
        // object has gone inactive but our last send was moving --> send non-moving update immediately
        return true;
    }

    _lastStep = simulationStep;
    if (glm::length2(_serverVelocity) > 0.0f) {
        // the entity-server doesn't know where avatars are, so it doesn't do simple extrapolation for children of
        // avatars.  We are trying to guess what values the entity server has, so we don't do it here, either.  See
        // related code in EntitySimulation::moveSimpleKinematics.
        bool ancestryIsKnown;
        _entity->getMaximumAACube(ancestryIsKnown);
        bool hasAvatarAncestor = _entity->hasAncestorOfType(NestableType::Avatar);

        if (ancestryIsKnown && !hasAvatarAncestor) {
            _serverVelocity += _serverAcceleration * dt;
            _serverVelocity *= powf(1.0f - _body->getLinearDamping(), dt);

            // NOTE: we ignore the second-order acceleration term when integrating
            // the position forward because Bullet also does this.
            _serverPosition += dt * _serverVelocity;
        }
    }

    if (_entity->dynamicDataNeedsTransmit()) {
        _outgoingPriority = _entity->hasActions() ? SCRIPT_GRAB_SIMULATION_PRIORITY : SCRIPT_POKE_SIMULATION_PRIORITY;
        return true;
    }

    if (_entity->shouldSuppressLocationEdits()) {
        return false;
    }

    // Else we measure the error between current and extrapolated transform (according to expected behavior
    // of remote EntitySimulation) and return true if the error is significant.

    // NOTE: math is done in the simulation-frame, which is NOT necessarily the same as the world-frame
    // due to _worldOffset.
    // TODO: compensate for _worldOffset offset here

    // compute position error

    btTransform worldTrans = _body->getWorldTransform();
    glm::vec3 position = worldToLocal.transform(bulletToGLM(worldTrans.getOrigin()));

    float dx2 = glm::distance2(position, _serverPosition);
    const float MAX_POSITION_ERROR_SQUARED = 0.000004f; // corresponds to 2mm
    if (dx2 > MAX_POSITION_ERROR_SQUARED) {
        // we don't mind larger position error when the object has high speed
        // so we divide by speed and check again
        float speed2 = glm::length2(_serverVelocity);
        const float MIN_ERROR_RATIO_SQUARED = 0.0025f; // corresponds to 5% error in 1 second
        const float MIN_SPEED_SQUARED = 1.0e-6f; // corresponds to 1mm/sec
        if (speed2 < MIN_SPEED_SQUARED || dx2 / speed2 > MIN_ERROR_RATIO_SQUARED) {
            #ifdef WANT_DEBUG
                qCDebug(physics) << ".... (dx2 > MAX_POSITION_ERROR_SQUARED) ....";
                qCDebug(physics) << "wasPosition:" << wasPosition;
                qCDebug(physics) << "bullet position:" << position;
                qCDebug(physics) << "_serverPosition:" << _serverPosition;
                qCDebug(physics) << "dx2:" << dx2;
            #endif
            return true;
        }
    }

    if (glm::length2(_serverAngularVelocity) > 0.0f) {
        // compute rotation error
        float attenuation = powf(1.0f - _body->getAngularDamping(), dt);
        _serverAngularVelocity *= attenuation;

        // Bullet caps the effective rotation velocity inside its rotation integration step, therefore
        // we must integrate with the same algorithm and timestep in order achieve similar results.
        for (int i = 0; i < numSteps; ++i) {
            _serverRotation = glm::normalize(computeBulletRotationStep(_serverAngularVelocity,
                                                                       PHYSICS_ENGINE_FIXED_SUBSTEP) * _serverRotation);
        }
    }
    const float MIN_ROTATION_DOT = 0.99999f; // This corresponds to about 0.5 degrees of rotation
    glm::quat actualRotation = worldToLocal.getRotation() * bulletToGLM(worldTrans.getRotation());

    #ifdef WANT_DEBUG
        if ((fabsf(glm::dot(actualRotation, _serverRotation)) < MIN_ROTATION_DOT)) {
            qCDebug(physics) << ".... ((fabsf(glm::dot(actualRotation, _serverRotation)) < MIN_ROTATION_DOT)) ....";

            qCDebug(physics) << "wasAngularVelocity:" << wasAngularVelocity;
            qCDebug(physics) << "_serverAngularVelocity:" << _serverAngularVelocity;

            qCDebug(physics) << "length wasAngularVelocity:" << glm::length(wasAngularVelocity);
            qCDebug(physics) << "length _serverAngularVelocity:" << glm::length(_serverAngularVelocity);

            qCDebug(physics) << "wasRotation:" << wasRotation;
            qCDebug(physics) << "bullet actualRotation:" << actualRotation;
            qCDebug(physics) << "_serverRotation:" << _serverRotation;
        }
    #endif

    return (fabsf(glm::dot(actualRotation, _serverRotation)) < MIN_ROTATION_DOT);
}

bool EntityMotionState::shouldSendUpdate(uint32_t simulationStep) {
    // NOTE: we expect _entity and _body to be valid in this context, since shouldSendUpdate() is only called
    // after doesNotNeedToSendUpdate() returns false and that call should return 'true' if _entity or _body are NULL.
    assert(_entity);
    assert(_body);
    assert(entityTreeIsLocked());

    if (_entity->getClientOnly() && _entity->getOwningAvatarID() != Physics::getSessionUUID()) {
        // don't send updates for someone else's avatarEntities
        return false;
    }

    if (_entity->dynamicDataNeedsTransmit() || _entity->queryAACubeNeedsUpdate()) {
        return true;
    }

    if (!isLocallyOwned()) {
        // we don't own the simulation

        // NOTE: we do not volunteer to own kinematic or static objects
        uint8_t insufficientPriority = _body->isStaticOrKinematicObject() ? VOLUNTEER_SIMULATION_PRIORITY : 0;

        bool shouldBid = _outgoingPriority > insufficientPriority && // but we would like to own it AND
                usecTimestampNow() > _nextOwnershipBid; // it is time to bid again
        if (shouldBid && _outgoingPriority < _entity->getSimulationPriority()) {
            // we are insufficiently interested so clear our interest
            // and reset the bid expiry
            _outgoingPriority = 0;
            _nextOwnershipBid = usecTimestampNow() + USECS_BETWEEN_OWNERSHIP_BIDS;
        }
        return shouldBid;
    }

    return remoteSimulationOutOfSync(simulationStep);
}

void EntityMotionState::sendUpdate(OctreeEditPacketSender* packetSender, uint32_t step) {
    assert(_entity);
    assert(entityTreeIsLocked());

    if (!_body->isActive()) {
        // make sure all derivatives are zero
        _entity->setVelocity(Vectors::ZERO);
        _entity->setAngularVelocity(Vectors::ZERO);
        _entity->setAcceleration(Vectors::ZERO);
        _numInactiveUpdates++;
    } else {
        glm::vec3 gravity = _entity->getGravity();

        // if this entity has been accelerated at close to gravity for a certain number of simulation-steps, let
        // the entity server's estimates include gravity.
        const uint8_t STEPS_TO_DECIDE_BALLISTIC = 4;
        if (_accelerationNearlyGravityCount >= STEPS_TO_DECIDE_BALLISTIC) {
            _entity->setAcceleration(gravity);
        } else {
            _entity->setAcceleration(Vectors::ZERO);
        }

        if (!_body->isStaticOrKinematicObject()) {
            const float DYNAMIC_LINEAR_VELOCITY_THRESHOLD = 0.05f;  // 5 cm/sec
            const float DYNAMIC_ANGULAR_VELOCITY_THRESHOLD = 0.087266f;  // ~5 deg/sec

            bool movingSlowlyLinear =
                glm::length2(_entity->getVelocity()) < (DYNAMIC_LINEAR_VELOCITY_THRESHOLD * DYNAMIC_LINEAR_VELOCITY_THRESHOLD);
            bool movingSlowlyAngular = glm::length2(_entity->getAngularVelocity()) <
                    (DYNAMIC_ANGULAR_VELOCITY_THRESHOLD * DYNAMIC_ANGULAR_VELOCITY_THRESHOLD);
            bool movingSlowly = movingSlowlyLinear && movingSlowlyAngular && _entity->getAcceleration() == Vectors::ZERO;

            if (movingSlowly) {
                // velocities might not be zero, but we'll fake them as such, which will hopefully help convince
                // other simulating observers to deactivate their own copies
                glm::vec3 zero(0.0f);
                _entity->setVelocity(zero);
                _entity->setAngularVelocity(zero);
            }
        }
        _numInactiveUpdates = 0;
    }

    // remember properties for local server prediction
    Transform localTransform;
    _entity->getLocalTransformAndVelocities(localTransform, _serverVelocity, _serverAngularVelocity);
    _serverPosition = localTransform.getTranslation();
    _serverRotation = localTransform.getRotation();
    _serverAcceleration = _entity->getAcceleration();
    _serverActionData = _entity->getDynamicData();

    EntityItemProperties properties;

    // explicitly set the properties that changed so that they will be packed
    properties.setPosition(_entity->getLocalPosition());
    properties.setRotation(_entity->getLocalOrientation());

    properties.setVelocity(_serverVelocity);
    properties.setAcceleration(_serverAcceleration);
    properties.setAngularVelocity(_serverAngularVelocity);
    if (_entity->dynamicDataNeedsTransmit()) {
        _entity->setDynamicDataNeedsTransmit(false);
        properties.setActionData(_serverActionData);
    }

    if (properties.transformChanged()) {
        if (_entity->checkAndMaybeUpdateQueryAACube()) {
            // due to parenting, the server may not know where something is in world-space, so include the bounding cube.
            properties.setQueryAACube(_entity->getQueryAACube());
        }
    }

    // set the LastEdited of the properties but NOT the entity itself
    quint64 now = usecTimestampNow();
    properties.setLastEdited(now);

    #ifdef WANT_DEBUG
        quint64 lastSimulated = _entity->getLastSimulated();
        qCDebug(physics) << "EntityMotionState::sendUpdate()";
        qCDebug(physics) << "        EntityItemId:" << _entity->getEntityItemID()
                         << "---------------------------------------------";
        qCDebug(physics) << "       lastSimulated:" << debugTime(lastSimulated, now);
    #endif //def WANT_DEBUG

    if (_numInactiveUpdates > 0) {
        // we own the simulation but the entity has stopped so we tell the server we're clearing simulatorID
        // but we remember we do still own it...  and rely on the server to tell us we don't
        properties.clearSimulationOwner();
        _outgoingPriority = 0;
        _entity->setPendingOwnershipPriority(_outgoingPriority, now);
    } else if (!isLocallyOwned()) {
        // we don't own the simulation for this entity yet, but we're sending a bid for it
        quint8 bidPriority = glm::max<uint8_t>(_outgoingPriority, VOLUNTEER_SIMULATION_PRIORITY);
        properties.setSimulationOwner(Physics::getSessionUUID(), bidPriority);
        _nextOwnershipBid = now + USECS_BETWEEN_OWNERSHIP_BIDS;
        // copy _outgoingPriority into pendingPriority...
        _entity->setPendingOwnershipPriority(_outgoingPriority, now);
        // don't forget to remember that we have made a bid
        _entity->rememberHasSimulationOwnershipBid();
        // ...then reset _outgoingPriority in preparation for the next frame
        _outgoingPriority = 0;
    } else if (_outgoingPriority != _entity->getSimulationPriority()) {
        // we own the simulation but our desired priority has changed
        if (_outgoingPriority == 0) {
            // we should release ownership
            properties.clearSimulationOwner();
        } else {
            // we just need to change the priority
            properties.setSimulationOwner(Physics::getSessionUUID(), _outgoingPriority);
        }
        _entity->setPendingOwnershipPriority(_outgoingPriority, now);
    }

    EntityItemID id(_entity->getID());
    EntityEditPacketSender* entityPacketSender = static_cast<EntityEditPacketSender*>(packetSender);
    #ifdef WANT_DEBUG
        qCDebug(physics) << "EntityMotionState::sendUpdate()... calling queueEditEntityMessage()...";
    #endif

    EntityTreeElementPointer element = _entity->getElement();
    EntityTreePointer tree = element ? element->getTree() : nullptr;

    properties.setClientOnly(_entity->getClientOnly());
    properties.setOwningAvatarID(_entity->getOwningAvatarID());

    entityPacketSender->queueEditEntityMessage(PacketType::EntityPhysics, tree, id, properties);
    _entity->setLastBroadcast(now);

    // if we've moved an entity with children, check/update the queryAACube of all descendents and tell the server
    // if they've changed.
    _entity->forEachDescendant([&](SpatiallyNestablePointer descendant) {
        if (descendant->getNestableType() == NestableType::Entity) {
            EntityItemPointer entityDescendant = std::static_pointer_cast<EntityItem>(descendant);
            if (descendant->checkAndMaybeUpdateQueryAACube()) {
                EntityItemProperties newQueryCubeProperties;
                newQueryCubeProperties.setQueryAACube(descendant->getQueryAACube());
                newQueryCubeProperties.setLastEdited(properties.getLastEdited());

                newQueryCubeProperties.setClientOnly(entityDescendant->getClientOnly());
                newQueryCubeProperties.setOwningAvatarID(entityDescendant->getOwningAvatarID());

                entityPacketSender->queueEditEntityMessage(PacketType::EntityPhysics, tree,
                                                           descendant->getID(), newQueryCubeProperties);
                entityDescendant->setLastBroadcast(now);
            }
        }
    });

    _lastStep = step;
}

uint32_t EntityMotionState::getIncomingDirtyFlags() {
    assert(entityTreeIsLocked());
    uint32_t dirtyFlags = 0;
    if (_body && _entity) {
        dirtyFlags = _entity->getDirtyFlags();

        if (dirtyFlags & Simulation::DIRTY_SIMULATOR_ID) {
            // when SIMULATOR_ID changes we must check for reinterpretation of asymmetric collision mask
            // bits for the avatar groups (e.g. MY_AVATAR vs OTHER_AVATAR)
            uint8_t entityCollisionMask = _entity->getCollisionless() ? 0 : _entity->getCollisionMask();
            if ((bool)(entityCollisionMask & USER_COLLISION_GROUP_MY_AVATAR) !=
                    (bool)(entityCollisionMask & USER_COLLISION_GROUP_OTHER_AVATAR)) {
                // bits are asymmetric --> flag for reinsertion in physics simulation
                dirtyFlags |= Simulation::DIRTY_COLLISION_GROUP;
            }
        }
        // we add DIRTY_MOTION_TYPE if the body's motion type disagrees with entity velocity settings
        int bodyFlags = _body->getCollisionFlags();
        bool isMoving = _entity->isMovingRelativeToParent();

        if (((bodyFlags & btCollisionObject::CF_STATIC_OBJECT) && isMoving) // ||
            // TODO -- there is opportunity for an optimization here, but this currently causes
            // excessive re-insertion of the rigid body.
            // (bodyFlags & btCollisionObject::CF_KINEMATIC_OBJECT && !isMoving)
            ) {
            dirtyFlags |= Simulation::DIRTY_MOTION_TYPE;
        }
    }
    return dirtyFlags;
}

void EntityMotionState::clearIncomingDirtyFlags() {
    assert(entityTreeIsLocked());
    if (_body && _entity) {
        _entity->clearDirtyFlags();
    }
}

// virtual
uint8_t EntityMotionState::getSimulationPriority() const {
    return _entity->getSimulationPriority();
}

// virtual
QUuid EntityMotionState::getSimulatorID() const {
    assert(entityTreeIsLocked());
    return _entity->getSimulatorID();
}

void EntityMotionState::bump(uint8_t priority) {
    assert(priority != 0);
    upgradeOutgoingPriority(glm::max(VOLUNTEER_SIMULATION_PRIORITY, --priority));
}

void EntityMotionState::resetMeasuredBodyAcceleration() {
    _lastMeasureStep = ObjectMotionState::getWorldSimulationStep();
    if (_body) {
        _lastVelocity = getBodyLinearVelocityGTSigma();
    } else {
        _lastVelocity = Vectors::ZERO;
    }
    _measuredAcceleration = Vectors::ZERO;
}

void EntityMotionState::measureBodyAcceleration() {
    // try to manually measure the true acceleration of the object
    uint32_t thisStep = ObjectMotionState::getWorldSimulationStep();
    uint32_t numSubsteps = thisStep - _lastMeasureStep;
    if (numSubsteps > 0) {
        float dt = ((float)numSubsteps * PHYSICS_ENGINE_FIXED_SUBSTEP);
        float invDt = 1.0f / dt;
        _lastMeasureStep = thisStep;
        _measuredDeltaTime = dt;

        // Note: the integration equation for velocity uses damping:   v1 = (v0 + a * dt) * (1 - D)^dt
        // hence the equation for acceleration is: a = (v1 / (1 - D)^dt - v0) / dt
        glm::vec3 velocity = getBodyLinearVelocityGTSigma();

        _measuredAcceleration = (velocity / powf(1.0f - _body->getLinearDamping(), dt) - _lastVelocity) * invDt;
        _lastVelocity = velocity;
        if (numSubsteps > PHYSICS_ENGINE_MAX_NUM_SUBSTEPS) {
            _loopsWithoutOwner = 0;
            _lastStep = ObjectMotionState::getWorldSimulationStep();
            _numInactiveUpdates = 0;
        }

        glm::vec3 gravity = _entity->getGravity();
        float gravityLength = glm::length(gravity);
        float accVsGravity = glm::abs(glm::length(_measuredAcceleration) - gravityLength);
        const float ACCELERATION_EQUIVALENT_EPSILON_RATIO = 0.1f;
        if (accVsGravity < ACCELERATION_EQUIVALENT_EPSILON_RATIO * gravityLength) {
            // acceleration measured during the most recent simulation step was close to gravity.
            if (_accelerationNearlyGravityCount < (uint8_t)(-2)) {
                ++_accelerationNearlyGravityCount;
            }
        } else {
            // acceleration wasn't similar to this entities gravity, so reset the went-ballistic counter
            _accelerationNearlyGravityCount = 0;
        }
    }
}

glm::vec3 EntityMotionState::getObjectLinearVelocityChange() const {
    // This is the dampened change in linear velocity, as calculated in measureBodyAcceleration: dv = a * dt
    // It is generally only meaningful during the lifespan of collision. In particular, it is not meaningful
    // when the entity first starts moving via direct user action.
    return _measuredAcceleration * _measuredDeltaTime;
}

// virtual
void EntityMotionState::setMotionType(PhysicsMotionType motionType) {
    ObjectMotionState::setMotionType(motionType);
    resetMeasuredBodyAcceleration();
}


// virtual
QString EntityMotionState::getName() const {
    assert(entityTreeIsLocked());
    return _entity->getName();
}

// virtual
void EntityMotionState::computeCollisionGroupAndMask(int16_t& group, int16_t& mask) const {
    assert(_entity);
    _entity->computeCollisionGroupAndFinalMask(group, mask);
}

bool EntityMotionState::isLocallyOwned() const {
    return _entity->getSimulatorID() == Physics::getSessionUUID();
}

bool EntityMotionState::shouldBeLocallyOwned() const {
    return (_outgoingPriority > VOLUNTEER_SIMULATION_PRIORITY && _outgoingPriority > _entity->getSimulationPriority()) ||
        _entity->getSimulatorID() == Physics::getSessionUUID();
}

void EntityMotionState::upgradeOutgoingPriority(uint8_t priority) {
    _outgoingPriority = glm::max<uint8_t>(_outgoingPriority, priority);
}
