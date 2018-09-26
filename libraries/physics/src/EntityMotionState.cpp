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

#include "EntityMotionState.h"

#include <glm/gtx/norm.hpp>

#include <EntityItem.h>
#include <EntityItemProperties.h>
#include <EntityEditPacketSender.h>
#include <LogHandler.h>
#include <PhysicsCollisionGroups.h>
#include <Profile.h>

#include "BulletUtil.h"
#include "PhysicsEngine.h"
#include "PhysicsHelpers.h"
#include "PhysicsLogging.h"

#ifdef WANT_DEBUG_ENTITY_TREE_LOCKS
#include "EntityTree.h"

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

const uint8_t LOOPS_FOR_SIMULATION_ORPHAN = 50;
const quint64 USECS_BETWEEN_OWNERSHIP_BIDS = USECS_PER_SECOND / 5;


EntityMotionState::EntityMotionState(btCollisionShape* shape, EntityItemPointer entity) :
    ObjectMotionState(nullptr),
    _entity(entity),
    _serverPosition(0.0f),
    _serverRotation(),
    _serverVelocity(0.0f),
    _serverAngularVelocity(0.0f),
    _serverGravity(0.0f),
    _serverAcceleration(0.0f),
    _serverActionData(QByteArray()),
    _lastVelocity(0.0f),
    _measuredAcceleration(0.0f),
    _nextBidExpiry(0),
    _measuredDeltaTime(0.0f),
    _lastMeasureStep(0),
    _lastStep(0),
    _loopsWithoutOwner(0),
    _accelerationNearlyGravityCount(0),
    _numInactiveUpdates(1)
{
    // Why is _numInactiveUpdates initialied to 1?
    // Because: when an entity is first created by a LOCAL operatioin its local simulation ownership is assumed,
    // which causes it to be immediately placed on the 'owned' list, but in this case an "update" already just
    // went out for the object's creation and there is no need to send another.  By initializing _numInactiveUpdates
    // to 1 here we trick remoteSimulationOutOfSync() to return "false" the first time through for this case.

    _type = MOTIONSTATE_TYPE_ENTITY;
    assert(_entity);
    assert(entityTreeIsLocked());
    setMass(_entity->computeMass());
    // we need the side-effects of EntityMotionState::setShape() so we call it explicitly here
    // rather than pass the legit shape pointer to the ObjectMotionState ctor above.
    setShape(shape);

    if (_entity->getClientOnly() && _entity->getOwningAvatarID() != Physics::getSessionUUID()) {
        // client-only entities are always thus, so we cache this fact in _ownershipState
        _ownershipState = EntityMotionState::OwnershipState::Unownable;
    }

    Transform localTransform;
    _entity->getLocalTransformAndVelocities(localTransform, _serverVelocity, _serverAngularVelocity);
    _serverPosition = localTransform.getTranslation();
    _serverRotation = localTransform.getRotation();
    _serverAcceleration = _entity->getAcceleration();
    _serverActionData = _entity->getDynamicData();

}

EntityMotionState::~EntityMotionState() {
    if (_entity) {
        assert(_entity->getPhysicsInfo() == this);
        _entity->setPhysicsInfo(nullptr);
        _entity.reset();
    }
}

void EntityMotionState::updateServerPhysicsVariables() {
    if (_ownershipState != EntityMotionState::OwnershipState::LocallyOwned) {
        // only slam these values if we are NOT the simulation owner
        Transform localTransform;
        _entity->getLocalTransformAndVelocities(localTransform, _serverVelocity, _serverAngularVelocity);
        _serverPosition = localTransform.getTranslation();
        _serverRotation = localTransform.getRotation();
        _serverAcceleration = _entity->getAcceleration();
        _serverActionData = _entity->getDynamicData();
        _lastStep = ObjectMotionState::getWorldSimulationStep();
    }
}

void EntityMotionState::handleDeactivation() {
    // copy _server data to entity
    Transform localTransform = _entity->getLocalTransform();
    localTransform.setTranslation(_serverPosition);
    localTransform.setRotation(_serverRotation);
    _entity->setLocalTransformAndVelocities(localTransform, ENTITY_ITEM_ZERO_VEC3, ENTITY_ITEM_ZERO_VEC3);
    // and also to RigidBody
    btTransform worldTrans;
    worldTrans.setOrigin(glmToBullet(_entity->getWorldPosition()));
    worldTrans.setRotation(glmToBullet(_entity->getWorldOrientation()));
    _body->setWorldTransform(worldTrans);
    // no need to update velocities... should already be zero
}

// virtual
void EntityMotionState::handleEasyChanges(uint32_t& flags) {
    assert(entityTreeIsLocked());
    updateServerPhysicsVariables();
    ObjectMotionState::handleEasyChanges(flags);

    if (flags & Simulation::DIRTY_SIMULATOR_ID) {
        if (_entity->getSimulatorID().isNull()) {
            // simulation ownership has been removed
            if (glm::length2(_entity->getWorldVelocity()) == 0.0f) {
            // TODO: also check angularVelocity
                // this object is coming to rest
                flags &= ~Simulation::DIRTY_PHYSICS_ACTIVATION;
                _body->setActivationState(WANTS_DEACTIVATION);
                const float ACTIVATION_EXPIRY = 3.0f; // something larger than the 2.0 hard coded in Bullet
                _body->setDeactivationTime(ACTIVATION_EXPIRY);
            } else {
                // disowned object is still moving --> start timer for ownership bid
                // TODO? put a delay in here proportional to distance from object?
                _bumpedPriority = glm::max(_bumpedPriority, VOLUNTEER_SIMULATION_PRIORITY);
                _nextBidExpiry = usecTimestampNow() + USECS_BETWEEN_OWNERSHIP_BIDS;
            }
            _loopsWithoutOwner = 0;
            _numInactiveUpdates = 0;
        } else if (!isLocallyOwned()) {
            // the entity is owned by someone else
            _nextBidExpiry = usecTimestampNow() + USECS_BETWEEN_OWNERSHIP_BIDS;
            _numInactiveUpdates = 0;
        }
    }
    if (flags & Simulation::DIRTY_SIMULATION_OWNERSHIP_PRIORITY) {
        // The DIRTY_SIMULATOR_OWNERSHIP_PRIORITY bit means one of the following:
        // (1) we own it but may need to change the priority OR...
        // (2) we don't own it but should bid (because a local script has been changing physics properties)
        // reset bid expiry so that we bid ASAP
        _nextBidExpiry = 0;
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
    if (_motionType == MOTION_TYPE_KINEMATIC) {
        BT_PROFILE("kinematicIntegration");
        // This is physical kinematic motion which steps strictly by the subframe count
        // of the physics simulation and uses full gravity for acceleration.
        _entity->setAcceleration(_entity->getGravity());

        uint32_t thisStep = ObjectMotionState::getWorldSimulationStep();
        float dt = (thisStep - _lastKinematicStep) * PHYSICS_ENGINE_FIXED_SUBSTEP;
        _lastKinematicStep = thisStep;
        _entity->stepKinematicMotion(dt);

        // and set the acceleration-matches-gravity count high so that if we send an update
        // it will use the correct acceleration for remote simulations
        _accelerationNearlyGravityCount = (uint8_t)(-1);
    }
    worldTrans.setOrigin(glmToBullet(getObjectPosition()));
    worldTrans.setRotation(glmToBullet(_entity->getWorldOrientation()));
}

// This callback is invoked by the physics simulation at the end of each simulation step...
// iff the corresponding RigidBody is DYNAMIC and ACTIVE.
void EntityMotionState::setWorldTransform(const btTransform& worldTrans) {
    assert(entityTreeIsLocked());
    measureBodyAcceleration();

    // If transform or velocities are flagged as dirty it means a network or scripted change
    // occured between the beginning and end of the stepSimulation() and we DON'T want to apply
    // these physics simulation results.
    uint32_t flags = _entity->getDirtyFlags() & (Simulation::DIRTY_TRANSFORM | Simulation::DIRTY_VELOCITIES);
    if (!flags) {
        // flags are clear
        _entity->setWorldTransform(bulletToGLM(worldTrans.getOrigin()), bulletToGLM(worldTrans.getRotation()));
        _entity->setWorldVelocity(getBodyLinearVelocity());
        _entity->setWorldAngularVelocity(getBodyAngularVelocity());
        _entity->setLastSimulated(usecTimestampNow());
    } else {
        // only set properties NOT flagged
        if (!(flags & Simulation::DIRTY_TRANSFORM)) {
            _entity->setWorldTransform(bulletToGLM(worldTrans.getOrigin()), bulletToGLM(worldTrans.getRotation()));
        }
        if (!(flags & Simulation::DIRTY_LINEAR_VELOCITY)) {
            _entity->setWorldVelocity(getBodyLinearVelocity());
        }
        if (!(flags & Simulation::DIRTY_ANGULAR_VELOCITY)) {
            _entity->setWorldAngularVelocity(getBodyAngularVelocity());
        }
        if (flags != (Simulation::DIRTY_TRANSFORM | Simulation::DIRTY_VELOCITIES)) {
            _entity->setLastSimulated(usecTimestampNow());
        }
    }

    if (_entity->getSimulatorID().isNull()) {
        _loopsWithoutOwner++;
        if (_loopsWithoutOwner > LOOPS_FOR_SIMULATION_ORPHAN && usecTimestampNow() > _nextBidExpiry) {
            _bumpedPriority = glm::max(_bumpedPriority, VOLUNTEER_SIMULATION_PRIORITY);
        }
    }
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

bool EntityMotionState::remoteSimulationOutOfSync(uint32_t simulationStep) {
    // NOTE: this method is only ever called when the entity simulation is locally owned
    DETAILED_PROFILE_RANGE(simulation_physics, "CheckOutOfSync");

    // Since we own the simulation: make sure _bidPriority is not less than current owned priority
    // because: a _bidPriority of zero indicates that we should drop ownership in the send.
    // TODO: need to be able to detect when logic dictates we *decrease* priority
    // WIP: print info whenever _bidPriority mismatches what is known to the entity

    if (_entity->dynamicDataNeedsTransmit()) {
        return true;
    }

    int numSteps = simulationStep - _lastStep;
    float dt = (float)(numSteps) * PHYSICS_ENGINE_FIXED_SUBSTEP;

    if (_numInactiveUpdates > 0) {
        const uint8_t MAX_NUM_INACTIVE_UPDATES = 20;
        if (_numInactiveUpdates > MAX_NUM_INACTIVE_UPDATES) {
            // clear local ownership (stop sending updates) and let the server clear itself
            _entity->clearSimulationOwnership();
            return false;
        }
        // we resend the inactive update with a growing delay: every INACTIVE_UPDATE_PERIOD * _numInactiveUpdates
        // until it is removed from the owned list
        // (which happens when we no longer own the simulation)
        const float INACTIVE_UPDATE_PERIOD = 0.5f;
        return (dt > INACTIVE_UPDATE_PERIOD * (float)_numInactiveUpdates);
    }

    if (!_body->isActive()) {
        // object has gone inactive but our last send was moving --> send non-moving update immediately
        return true;
    }

    if (usecTimestampNow() > _entity->getSimulationOwnershipExpiry()) {
        // send update every so often else server will revoke our ownership
        return true;
    }

    if (_body->isStaticOrKinematicObject()) {
        return false;
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

    // Else we measure the error between current and extrapolated transform (according to expected behavior
    // of remote EntitySimulation) and return true if the error is significant.

    // NOTE: math is done in the simulation-frame, which is NOT necessarily the same as the world-frame
    // due to _worldOffset.
    // TODO: compensate for _worldOffset offset here

    // compute position error
    bool parentTransformSuccess;
    Transform localToWorld = _entity->getParentTransform(parentTransformSuccess);
    Transform worldToLocal;
    if (parentTransformSuccess) {
        localToWorld.evalInverse(worldToLocal);
    }

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
            return true;
        }
    }

    if (glm::length2(_serverAngularVelocity) > 0.0f) {
        // compute rotation error
        //

        // Bullet caps the effective rotation velocity inside its rotation integration step, therefore
        // we must integrate with the same algorithm and timestep in order achieve similar results.
        float attenuation = powf(1.0f - _body->getAngularDamping(), PHYSICS_ENGINE_FIXED_SUBSTEP);
        _serverAngularVelocity *= attenuation;
        glm::quat rotation = computeBulletRotationStep(_serverAngularVelocity, PHYSICS_ENGINE_FIXED_SUBSTEP);
        for (int i = 1; i < numSteps; ++i) {
            _serverAngularVelocity *= attenuation;
            rotation = computeBulletRotationStep(_serverAngularVelocity, PHYSICS_ENGINE_FIXED_SUBSTEP) * rotation;
        }
        _serverRotation = glm::normalize(rotation * _serverRotation);
        const float MIN_ROTATION_DOT = 0.99999f; // This corresponds to about 0.5 degrees of rotation
        glm::quat actualRotation = worldToLocal.getRotation() * bulletToGLM(worldTrans.getRotation());
        return (fabsf(glm::dot(actualRotation, _serverRotation)) < MIN_ROTATION_DOT);
    }
    return false;
}

bool EntityMotionState::shouldSendUpdate(uint32_t simulationStep) {
    // NOTE: this method is only ever called when the entity simulation is locally owned
    DETAILED_PROFILE_RANGE(simulation_physics, "ShouldSend");
    // NOTE: we expect _entity and _body to be valid in this context, since shouldSendUpdate() is only called
    // after doesNotNeedToSendUpdate() returns false and that call should return 'true' if _entity or _body are NULL.
    assert(entityTreeIsLocked());

    // this case is prevented by setting _ownershipState to UNOWNABLE in EntityMotionState::ctor
    assert(!(_entity->getClientOnly() && _entity->getOwningAvatarID() != Physics::getSessionUUID()));

    if (_entity->dynamicDataNeedsTransmit()) {
        return true;
    }
    if (_entity->shouldSuppressLocationEdits()) {
        // "shouldSuppressLocationEdits" really means: "the entity has a 'Hold' action therefore
        // we don't need send an update unless the entity is not contained by its queryAACube"
        return _entity->queryAACubeNeedsUpdate();
    }

    return remoteSimulationOutOfSync(simulationStep);
}

void EntityMotionState::updateSendVelocities() {
    if (!_body->isActive()) {
        if (!_body->isKinematicObject()) {
            clearObjectVelocities();
        }
        // we pretend we sent the inactive update for this object
        _numInactiveUpdates = 1;
    } else {
        glm::vec3 gravity = _entity->getGravity();

        // if this entity has been accelerated at close to gravity for a certain number of simulation-steps
        // let the entity server's estimates include gravity.
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
                glm::length2(_entity->getWorldVelocity()) < (DYNAMIC_LINEAR_VELOCITY_THRESHOLD * DYNAMIC_LINEAR_VELOCITY_THRESHOLD);
            bool movingSlowlyAngular = glm::length2(_entity->getWorldAngularVelocity()) <
                    (DYNAMIC_ANGULAR_VELOCITY_THRESHOLD * DYNAMIC_ANGULAR_VELOCITY_THRESHOLD);
            bool movingSlowly = movingSlowlyLinear && movingSlowlyAngular && _entity->getAcceleration() == Vectors::ZERO;

            if (movingSlowly) {
                // velocities might not be zero, but we'll fake them as such, which will hopefully help convince
                // other simulating observers to deactivate their own copies
                clearObjectVelocities();
            }
        }
        _numInactiveUpdates = 0;
    }
}

void EntityMotionState::sendBid(OctreeEditPacketSender* packetSender, uint32_t step) {
    DETAILED_PROFILE_RANGE(simulation_physics, "Bid");
    assert(entityTreeIsLocked());

    updateSendVelocities();

    EntityItemProperties properties;
    Transform localTransform;
    glm::vec3 linearVelocity;
    glm::vec3 angularVelocity;
    _entity->getLocalTransformAndVelocities(localTransform, linearVelocity, angularVelocity);
    properties.setPosition(localTransform.getTranslation());
    properties.setRotation(localTransform.getRotation());
    properties.setVelocity(linearVelocity);
    properties.setAcceleration(_entity->getAcceleration());
    properties.setAngularVelocity(angularVelocity);
    if (_entity->dynamicDataNeedsTransmit()) {
        _entity->setDynamicDataNeedsTransmit(false);
        properties.setActionData(_entity->getDynamicData());
    }

    if (_entity->updateQueryAACube()) {
        // due to parenting, the server may not know where something is in world-space, so include the bounding cube.
        properties.setQueryAACube(_entity->getQueryAACube());
    }

    // set the LastEdited of the properties but NOT the entity itself
    quint64 now = usecTimestampNow();
    properties.setLastEdited(now);

    // we don't own the simulation for this entity yet, but we're sending a bid for it
    uint8_t finalBidPriority = computeFinalBidPriority();
    _entity->clearScriptSimulationPriority();
    properties.setSimulationOwner(Physics::getSessionUUID(), finalBidPriority);
    _entity->setPendingOwnershipPriority(finalBidPriority);

    EntityTreeElementPointer element = _entity->getElement();
    EntityTreePointer tree = element ? element->getTree() : nullptr;

    properties.setClientOnly(_entity->getClientOnly());
    properties.setOwningAvatarID(_entity->getOwningAvatarID());

    EntityItemID id(_entity->getID());
    EntityEditPacketSender* entityPacketSender = static_cast<EntityEditPacketSender*>(packetSender);
    entityPacketSender->queueEditEntityMessage(PacketType::EntityPhysics, tree, id, properties);
    _entity->setLastBroadcast(now); // for debug/physics status icons

    // NOTE: we don't descend to children for ownership bid.  Instead, if we win ownership of the parent
    // then in sendUpdate() we'll walk descendents and send updates for their QueryAACubes if necessary.

    _lastStep = step;
    _nextBidExpiry = now + USECS_BETWEEN_OWNERSHIP_BIDS;

    // after sending a bid/update we clear _bumpedPriority
    // which might get promoted again next frame (after local script or simulation interaction)
    // or we might win the bid
    _bumpedPriority = 0;
}

void EntityMotionState::sendUpdate(OctreeEditPacketSender* packetSender, uint32_t step) {
    DETAILED_PROFILE_RANGE(simulation_physics, "Send");
    assert(entityTreeIsLocked());
    assert(isLocallyOwned());

    updateSendVelocities();

    // remember _serverFoo data for local prediction of server state
    Transform localTransform;
    _entity->getLocalTransformAndVelocities(localTransform, _serverVelocity, _serverAngularVelocity);
    _serverPosition = localTransform.getTranslation();
    _serverRotation = localTransform.getRotation();
    _serverAcceleration = _entity->getAcceleration();
    _serverActionData = _entity->getDynamicData();

    EntityItemProperties properties;
    properties.setPosition(_entity->getLocalPosition());
    properties.setRotation(_entity->getLocalOrientation());
    properties.setVelocity(_serverVelocity);
    properties.setAcceleration(_serverAcceleration);
    properties.setAngularVelocity(_serverAngularVelocity);
    if (_entity->dynamicDataNeedsTransmit()) {
        _entity->setDynamicDataNeedsTransmit(false);
        properties.setActionData(_serverActionData);
    }

    if (_entity->updateQueryAACube()) {
        // due to parenting, the server may not know where something is in world-space, so include the bounding cube.
        properties.setQueryAACube(_entity->getQueryAACube());
    }

    // set the LastEdited of the properties but NOT the entity itself
    quint64 now = usecTimestampNow();
    properties.setLastEdited(now);
    _entity->setSimulationOwnershipExpiry(now + MAX_OUTGOING_SIMULATION_UPDATE_PERIOD);

    if (_numInactiveUpdates > 0 && _entity->getScriptSimulationPriority() == 0) {
        // the entity is stopped and inactive so we tell the server we're clearing simulatorID
        // but we remember we do still own it...  and rely on the server to tell us we don't
        properties.clearSimulationOwner();
        _entity->setPendingOwnershipPriority(0);
    } else {
        uint8_t newPriority = computeFinalBidPriority();
        _entity->clearScriptSimulationPriority();
        // if we get here then we own the simulation and the object is NOT going inactive
        // if newPriority is zero, then it must be outside of R1, which means we should really set it to YIELD
        // which we achive by just setting it to the max of the two
        newPriority = glm::max(newPriority, YIELD_SIMULATION_PRIORITY);
        if (newPriority != _entity->getSimulationPriority() &&
                !(newPriority == VOLUNTEER_SIMULATION_PRIORITY && _entity->getSimulationPriority() == RECRUIT_SIMULATION_PRIORITY)) {
            // our desired priority has changed
            if (newPriority == 0) {
                // we should release ownership
                properties.clearSimulationOwner();
            } else {
                // we just need to inform the entity-server
                properties.setSimulationOwner(Physics::getSessionUUID(), newPriority);
            }
            _entity->setPendingOwnershipPriority(newPriority);
        }
    }

    EntityItemID id(_entity->getID());
    EntityEditPacketSender* entityPacketSender = static_cast<EntityEditPacketSender*>(packetSender);

    EntityTreeElementPointer element = _entity->getElement();
    EntityTreePointer tree = element ? element->getTree() : nullptr;

    properties.setClientOnly(_entity->getClientOnly());
    properties.setOwningAvatarID(_entity->getOwningAvatarID());

    entityPacketSender->queueEditEntityMessage(PacketType::EntityPhysics, tree, id, properties);
    _entity->setLastBroadcast(now); // for debug/physics status icons

    // if we've moved an entity with children, check/update the queryAACube of all descendents and tell the server
    // if they've changed.
    _entity->forEachDescendant([&](SpatiallyNestablePointer descendant) {
        if (descendant->getNestableType() == NestableType::Entity) {
            EntityItemPointer entityDescendant = std::static_pointer_cast<EntityItem>(descendant);
            if (descendant->updateQueryAACube()) {
                EntityItemProperties newQueryCubeProperties;
                newQueryCubeProperties.setQueryAACube(descendant->getQueryAACube());
                newQueryCubeProperties.setLastEdited(properties.getLastEdited());
                newQueryCubeProperties.setClientOnly(entityDescendant->getClientOnly());
                newQueryCubeProperties.setOwningAvatarID(entityDescendant->getOwningAvatarID());

                entityPacketSender->queueEditEntityMessage(PacketType::EntityPhysics, tree,
                                                           descendant->getID(), newQueryCubeProperties);
                entityDescendant->setLastBroadcast(now); // for debug/physics status icons
            }
        }
    });

    _lastStep = step;

    // after sending a bid/update we clear _bumpedPriority
    // which might get promoted again next frame (after local script or simulation interaction)
    // or we might win the bid
    _bumpedPriority = 0;
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
        _entity->clearDirtyFlags(DIRTY_PHYSICS_FLAGS);
    }
}

// virtual
uint8_t EntityMotionState::getSimulationPriority() const {
    return _entity->getSimulationPriority();
}

void EntityMotionState::slaveBidPriority() {
    _bumpedPriority = glm::max(_bumpedPriority, _entity->getSimulationPriority());
}

// virtual
QUuid EntityMotionState::getSimulatorID() const {
    assert(entityTreeIsLocked());
    return _entity->getSimulatorID();
}

void EntityMotionState::bump(uint8_t priority) {
    assert(priority != 0);
    _bumpedPriority = glm::max(_bumpedPriority, --priority);
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
    DETAILED_PROFILE_RANGE(simulation_physics, "MeasureAccel");
    // try to manually measure the true acceleration of the object
    uint32_t thisStep = ObjectMotionState::getWorldSimulationStep();
    uint32_t numSubsteps = thisStep - _lastMeasureStep;
    if (numSubsteps > 0) {
        float dt = ((float)numSubsteps * PHYSICS_ENGINE_FIXED_SUBSTEP);
        float invDt = 1.0f / dt;
        _lastMeasureStep = thisStep;
        _measuredDeltaTime = dt;

        // Note: the integration equation for velocity uses damping (D):   v1 = (v0 + a * dt) * (1 - D)^dt
        // hence the equation for acceleration is: a = (v1 / (1 - D)^dt - v0) / dt
        glm::vec3 velocity = getBodyLinearVelocityGTSigma();

        const float MIN_DAMPING_FACTOR = 0.01f;
        float invDampingAttenuationFactor = 1.0f / glm::max(powf(1.0f - _body->getLinearDamping(), dt), MIN_DAMPING_FACTOR);
        _measuredAcceleration = (velocity * invDampingAttenuationFactor - _lastVelocity) * invDt;
        _lastVelocity = velocity;
        if (numSubsteps > PHYSICS_ENGINE_MAX_NUM_SUBSTEPS) {
            // we fall in here when _lastMeasureStep is old: the body has just become active
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
void EntityMotionState::computeCollisionGroupAndMask(int32_t& group, int32_t& mask) const {
    _entity->computeCollisionGroupAndFinalMask(group, mask);
}

bool EntityMotionState::shouldSendBid() const {
    // NOTE: this method is only ever called when the entity's simulation is NOT locally owned
    return _body->isActive()
        && (_region == workload::Region::R1)
        && glm::max(glm::max(VOLUNTEER_SIMULATION_PRIORITY, _bumpedPriority), _entity->getScriptSimulationPriority()) >= _entity->getSimulationPriority()
        && !_entity->getLocked();
}

uint8_t EntityMotionState::computeFinalBidPriority() const {
    return (_region == workload::Region::R1) ?
        glm::max(glm::max(VOLUNTEER_SIMULATION_PRIORITY, _bumpedPriority), _entity->getScriptSimulationPriority()) : 0;
}

bool EntityMotionState::isLocallyOwned() const {
    return _entity->getSimulatorID() == Physics::getSessionUUID();
}

bool EntityMotionState::isLocallyOwnedOrShouldBe() const {
    // this method could also be called "shouldGenerateCollisionEventForLocalScripts()"
    // because that is the only reason it's used
    if (_entity->getSimulatorID() == Physics::getSessionUUID()) {
        return true;
    } else {
        return computeFinalBidPriority() > glm::max(VOLUNTEER_SIMULATION_PRIORITY, _entity->getSimulationPriority());
    }
}

void EntityMotionState::setRegion(uint8_t region) {
    _region = region;
}

void EntityMotionState::initForBid() {
    assert(_ownershipState != EntityMotionState::OwnershipState::Unownable);
    _ownershipState = EntityMotionState::OwnershipState::PendingBid;
}

void EntityMotionState::initForOwned() {
    assert(_ownershipState != EntityMotionState::OwnershipState::Unownable);
    _ownershipState = EntityMotionState::OwnershipState::LocallyOwned;
}

void EntityMotionState::clearObjectVelocities() const {
    // If transform or velocities are flagged as dirty it means a network or scripted change
    // occured between the beginning and end of the stepSimulation() and we DON'T want to apply
    // these physics simulation results.
    uint32_t flags = _entity->getDirtyFlags() & (Simulation::DIRTY_TRANSFORM | Simulation::DIRTY_VELOCITIES);
    if (!flags) {
        _entity->setWorldVelocity(glm::vec3(0.0f));
        _entity->setWorldAngularVelocity(glm::vec3(0.0f));
    } else {
        if (!(flags & Simulation::DIRTY_LINEAR_VELOCITY)) {
            _entity->setWorldVelocity(glm::vec3(0.0f));
        }
        if (!(flags & Simulation::DIRTY_ANGULAR_VELOCITY)) {
            _entity->setWorldAngularVelocity(glm::vec3(0.0f));
        }
    }
    _entity->setAcceleration(glm::vec3(0.0f));
}

void EntityMotionState::saveKinematicState(btScalar timeStep) {
    _body->saveKinematicState(timeStep);

    // This is a WORKAROUND for a quirk in Bullet: due to floating point error slow spinning kinematic objects will
    // have a measured angular velocity of zero.  This probably isn't a bug that the Bullet team is interested in
    // fixing since there is one very simple workaround: use double-precision math for the physics simulation.
    // We're not ready migrate to double-precision yet so we explicitly work around it by slamming the RigidBody's
    // angular velocity with the value in the entity.
    _body->setAngularVelocity(glmToBullet(_entity->getWorldAngularVelocity()));
}
