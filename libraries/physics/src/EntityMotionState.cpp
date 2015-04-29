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

#include <EntityItem.h>
#include <EntityEditPacketSender.h>

#include "BulletUtil.h"
#include "EntityMotionState.h"
#include "PhysicsEngine.h"
#include "PhysicsHelpers.h"
#include "PhysicsLogging.h"

static const float ACCELERATION_EQUIVALENT_EPSILON_RATIO = 0.1f;
static const quint8 STEPS_TO_DECIDE_BALLISTIC = 4;


EntityMotionState::EntityMotionState(btCollisionShape* shape, EntityItem* entity) :
    ObjectMotionState(shape),
    _entity(entity),
    _sentMoving(false),
    _numNonMovingUpdates(0),
    _outgoingPacketFlags(DIRTY_PHYSICS_FLAGS),
    _sentStep(0),
    _sentPosition(0.0f),
    _sentRotation(),
    _sentVelocity(0.0f),
    _sentAngularVelocity(0.0f),
    _sentGravity(0.0f),
    _sentAcceleration(0.0f),
    _accelerationNearlyGravityCount(0),
    _shouldClaimSimulationOwnership(false),
    _movingStepsWithoutSimulationOwner(0)
{
    _type = MOTION_STATE_TYPE_ENTITY;
    assert(entity != nullptr);
}

EntityMotionState::~EntityMotionState() {
    assert(_entity);
    _entity = nullptr;
}

MotionType EntityMotionState::computeObjectMotionType() const {
    if (_entity->getCollisionsWillMove()) {
        return MOTION_TYPE_DYNAMIC;
    }
    return _entity->isMoving() ?  MOTION_TYPE_KINEMATIC : MOTION_TYPE_STATIC;
}

void EntityMotionState::updateKinematicState(uint32_t substep) {
    setKinematic(_entity->isMoving(), substep);
}

void EntityMotionState::stepKinematicSimulation(quint64 now) {
    assert(_isKinematic);
    // NOTE: this is non-physical kinematic motion which steps to real run-time (now)
    // which is different from physical kinematic motion (inside getWorldTransform())
    // which steps in physics simulation time.
    _entity->simulate(now);
    // TODO: we can't use measureBodyAcceleration() here because the entity
    // has no RigidBody and the timestep is a little bit out of sync with the physics simulation anyway.
    // Hence we must manually measure kinematic velocity and acceleration.
}

bool EntityMotionState::isMoving() const {
    return _entity->isMoving();
}

// This callback is invoked by the physics simulation in two cases:
// (1) when the RigidBody is first added to the world
//     (irregardless of MotionType: STATIC, DYNAMIC, or KINEMATIC)
// (2) at the beginning of each simulation step for KINEMATIC RigidBody's --
//     it is an opportunity for outside code to update the object's simulation position
void EntityMotionState::getWorldTransform(btTransform& worldTrans) const {
    if (_isKinematic) {
        // This is physical kinematic motion which steps strictly by the subframe count
        // of the physics simulation.
        uint32_t substep = PhysicsEngine::getNumSubsteps();
        float dt = (substep - _lastKinematicSubstep) * PHYSICS_ENGINE_FIXED_SUBSTEP;
        _entity->simulateKinematicMotion(dt);
        _entity->setLastSimulated(usecTimestampNow());

        // bypass const-ness so we can remember the substep
        const_cast<EntityMotionState*>(this)->_lastKinematicSubstep = substep;
    }
    worldTrans.setOrigin(glmToBullet(_entity->getPosition() - ObjectMotionState::getWorldOffset()));
    worldTrans.setRotation(glmToBullet(_entity->getRotation()));
}

// This callback is invoked by the physics simulation at the end of each simulation step...
// iff the corresponding RigidBody is DYNAMIC and has moved.
void EntityMotionState::setWorldTransform(const btTransform& worldTrans) {
    measureBodyAcceleration();
    _entity->setPosition(bulletToGLM(worldTrans.getOrigin()) + ObjectMotionState::getWorldOffset());
    _entity->setRotation(bulletToGLM(worldTrans.getRotation()));

    glm::vec3 v;
    getVelocity(v);
    _entity->setVelocity(v);

    glm::vec3 av;
    getAngularVelocity(av);
    _entity->setAngularVelocity(av);

    _entity->setLastSimulated(usecTimestampNow());

    if (_entity->getSimulatorID().isNull() && isMoving()) {
        // object is moving and has no owner.  attempt to claim simulation ownership.
        _movingStepsWithoutSimulationOwner++;
    } else {
        _movingStepsWithoutSimulationOwner = 0;
    }

    if (_movingStepsWithoutSimulationOwner > 4) { // XXX maybe meters from our characterController ?
        setShouldClaimSimulationOwnership(true);
    }

    _outgoingPacketFlags = DIRTY_PHYSICS_FLAGS;

    #ifdef WANT_DEBUG
        quint64 now = usecTimestampNow();
        qCDebug(physics) << "EntityMotionState::setWorldTransform()... changed entity:" << _entity->getEntityItemID();
        qCDebug(physics) << "       last edited:" << _entity->getLastEdited() << formatUsecTime(now - _entity->getLastEdited()) << "ago";
        qCDebug(physics) << "    last simulated:" << _entity->getLastSimulated() << formatUsecTime(now - _entity->getLastSimulated()) << "ago";
        qCDebug(physics) << "      last updated:" << _entity->getLastUpdated() << formatUsecTime(now - _entity->getLastUpdated()) << "ago";
    #endif
}

void EntityMotionState::updateBodyEasy(uint32_t flags, uint32_t step) {
    if (flags & (EntityItem::DIRTY_POSITION | EntityItem::DIRTY_VELOCITY | EntityItem::DIRTY_PHYSICS_NO_WAKE)) {
        if (flags & EntityItem::DIRTY_POSITION) {
            _sentPosition = getObjectPosition() - ObjectMotionState::getWorldOffset();
            btTransform worldTrans;
            worldTrans.setOrigin(glmToBullet(_sentPosition));

            _sentRotation = getObjectRotation();
            worldTrans.setRotation(glmToBullet(_sentRotation));

            _body->setWorldTransform(worldTrans);
        }
        if (flags & EntityItem::DIRTY_VELOCITY) {
            updateBodyVelocities();
        }
        _sentStep = step;

        if (flags & (EntityItem::DIRTY_POSITION | EntityItem::DIRTY_VELOCITY)) {
            _body->activate();
        }
    }

    if (flags & EntityItem::DIRTY_MATERIAL) {
        updateBodyMaterialProperties();
    }

    if (flags & EntityItem::DIRTY_MASS) {
        ShapeInfo shapeInfo;
        _entity->computeShapeInfo(shapeInfo);
        float mass = computeObjectMass(shapeInfo);
        btVector3 inertia(0.0f, 0.0f, 0.0f);
        _body->getCollisionShape()->calculateLocalInertia(mass, inertia);
        _body->setMassProps(mass, inertia);
        _body->updateInertiaTensor();
    }
}

void EntityMotionState::updateBodyMaterialProperties() {
    _body->setRestitution(getObjectRestitution());
    _body->setFriction(getObjectFriction());
    _body->setDamping(fabsf(btMin(getObjectLinearDamping(), 1.0f)), fabsf(btMin(getObjectAngularDamping(), 1.0f)));
}

void EntityMotionState::updateBodyVelocities() {
    if (_body) {
        _sentVelocity = getObjectLinearVelocity();
        setBodyVelocity(_sentVelocity);

        _sentAngularVelocity = getObjectAngularVelocity();
        setBodyAngularVelocity(_sentAngularVelocity);

        _sentGravity = getObjectGravity();
        setBodyGravity(_sentGravity);

        _body->setActivationState(ACTIVE_TAG);
    }
}

// RELIABLE_SEND_HACK: until we have truly reliable resends of non-moving updates
// we alwasy resend packets for objects that have stopped moving up to some max limit.
const int MAX_NUM_NON_MOVING_UPDATES = 5;

bool EntityMotionState::doesNotNeedToSendUpdate() const { 
    return !_body->isActive() && _numNonMovingUpdates > MAX_NUM_NON_MOVING_UPDATES;
}

bool EntityMotionState::remoteSimulationOutOfSync(uint32_t simulationStep) {
    assert(_body);
    // if we've never checked before, our _sentStep will be 0, and we need to initialize our state
    if (_sentStep == 0) {
        btTransform xform = _body->getWorldTransform();
        _sentPosition = bulletToGLM(xform.getOrigin());
        _sentRotation = bulletToGLM(xform.getRotation());
        _sentVelocity = bulletToGLM(_body->getLinearVelocity());
        _sentAngularVelocity = bulletToGLM(_body->getAngularVelocity());
        _sentStep = simulationStep;
        return false;
    }
    
    #ifdef WANT_DEBUG
    glm::vec3 wasPosition = _sentPosition;
    glm::quat wasRotation = _sentRotation;
    glm::vec3 wasAngularVelocity = _sentAngularVelocity;
    #endif

    int numSteps = simulationStep - _sentStep;
    float dt = (float)(numSteps) * PHYSICS_ENGINE_FIXED_SUBSTEP;
    _sentStep = simulationStep;
    bool isActive = _body->isActive();

    if (!isActive) {
        if (_sentMoving) { 
            // this object just went inactive so send an update immediately
            return true;
        } else {
            const float NON_MOVING_UPDATE_PERIOD = 1.0f;
            if (dt > NON_MOVING_UPDATE_PERIOD && _numNonMovingUpdates < MAX_NUM_NON_MOVING_UPDATES) {
                // RELIABLE_SEND_HACK: since we're not yet using a reliable method for non-moving update packets we repeat these
                // at a faster rate than the MAX period above, and only send a limited number of them.
                return true;
            }
        }
    }

    // Else we measure the error between current and extrapolated transform (according to expected behavior 
    // of remote EntitySimulation) and return true if the error is significant.

    // NOTE: math is done in the simulation-frame, which is NOT necessarily the same as the world-frame 
    // due to _worldOffset.

    // compute position error
    if (glm::length2(_sentVelocity) > 0.0f) {
        _sentVelocity += _sentAcceleration * dt;
        _sentVelocity *= powf(1.0f - _body->getLinearDamping(), dt);
        _sentPosition += dt * _sentVelocity;
    }

    btTransform worldTrans = _body->getWorldTransform();
    glm::vec3 position = bulletToGLM(worldTrans.getOrigin());
    
    float dx2 = glm::distance2(position, _sentPosition);

    const float MAX_POSITION_ERROR_SQUARED = 0.001f; // 0.001 m^2 ~~> 0.03 m
    if (dx2 > MAX_POSITION_ERROR_SQUARED) {

        #ifdef WANT_DEBUG
            qCDebug(physics) << ".... (dx2 > MAX_POSITION_ERROR_SQUARED) ....";
            qCDebug(physics) << "wasPosition:" << wasPosition;
            qCDebug(physics) << "bullet position:" << position;
            qCDebug(physics) << "_sentPosition:" << _sentPosition;
            qCDebug(physics) << "dx2:" << dx2;
        #endif

        return true;
    }
    
    if (glm::length2(_sentAngularVelocity) > 0.0f) {
        // compute rotation error
        float attenuation = powf(1.0f - _body->getAngularDamping(), dt);
        _sentAngularVelocity *= attenuation;
   
        // Bullet caps the effective rotation velocity inside its rotation integration step, therefore 
        // we must integrate with the same algorithm and timestep in order achieve similar results.
        for (int i = 0; i < numSteps; ++i) {
            _sentRotation = glm::normalize(computeBulletRotationStep(_sentAngularVelocity, PHYSICS_ENGINE_FIXED_SUBSTEP) * _sentRotation);
        }
    }
    const float MIN_ROTATION_DOT = 0.99f; // 0.99 dot threshold coresponds to about 16 degrees of slop
    glm::quat actualRotation = bulletToGLM(worldTrans.getRotation());

    #ifdef WANT_DEBUG
        if ((fabsf(glm::dot(actualRotation, _sentRotation)) < MIN_ROTATION_DOT)) {
            qCDebug(physics) << ".... ((fabsf(glm::dot(actualRotation, _sentRotation)) < MIN_ROTATION_DOT)) ....";
        
            qCDebug(physics) << "wasAngularVelocity:" << wasAngularVelocity;
            qCDebug(physics) << "_sentAngularVelocity:" << _sentAngularVelocity;

            qCDebug(physics) << "length wasAngularVelocity:" << glm::length(wasAngularVelocity);
            qCDebug(physics) << "length _sentAngularVelocity:" << glm::length(_sentAngularVelocity);

            qCDebug(physics) << "wasRotation:" << wasRotation;
            qCDebug(physics) << "bullet actualRotation:" << actualRotation;
            qCDebug(physics) << "_sentRotation:" << _sentRotation;
        }
    #endif

    return (fabsf(glm::dot(actualRotation, _sentRotation)) < MIN_ROTATION_DOT);
}

bool EntityMotionState::shouldSendUpdate(uint32_t simulationFrame) {
    if (!remoteSimulationOutOfSync(simulationFrame)) {
        return false;
    }

    if (getShouldClaimSimulationOwnership()) {
        return true;
    }

    auto nodeList = DependencyManager::get<NodeList>();
    const QUuid& myNodeID = nodeList->getSessionUUID();
    const QUuid& simulatorID = _entity->getSimulatorID();

    if (simulatorID != myNodeID) {
        // some other Node owns the simulating of this, so don't broadcast the results of local simulation.
        return false;
    }

    return true;
}

void EntityMotionState::sendUpdate(OctreeEditPacketSender* packetSender, uint32_t step) {
    if (!_entity->isKnownID()) {
        return; // never update entities that are unknown
    }
    if (_outgoingPacketFlags) {
        EntityItemProperties properties = _entity->getProperties();

        float gravityLength = glm::length(_entity->getGravity());
        float accVsGravity = glm::abs(glm::length(_measuredAcceleration) - gravityLength);
        if (accVsGravity < ACCELERATION_EQUIVALENT_EPSILON_RATIO * gravityLength) {
            // acceleration measured during the most recent simulation step was close to gravity.
            if (getAccelerationNearlyGravityCount() < STEPS_TO_DECIDE_BALLISTIC) {
                // only increment this if we haven't reached the threshold yet.  this is to avoid
                // overflowing the counter.
                incrementAccelerationNearlyGravityCount();
            }
        } else {
            // acceleration wasn't similar to this entities gravity, so reset the went-ballistic counter
            resetAccelerationNearlyGravityCount();
        }

        // if this entity has been accelerated at close to gravity for a certain number of simulation-steps, let
        // the entity server's estimates include gravity.
        if (getAccelerationNearlyGravityCount() >= STEPS_TO_DECIDE_BALLISTIC) {
            _entity->setAcceleration(_entity->getGravity());
        } else {
            _entity->setAcceleration(glm::vec3(0.0f));
        }

        if (_outgoingPacketFlags & EntityItem::DIRTY_POSITION) {
            btTransform worldTrans = _body->getWorldTransform();
            _sentPosition = bulletToGLM(worldTrans.getOrigin());
            properties.setPosition(_sentPosition + ObjectMotionState::getWorldOffset());
        
            _sentRotation = bulletToGLM(worldTrans.getRotation());
            properties.setRotation(_sentRotation);
        }

        bool zeroSpeed = true;
        bool zeroSpin = true;

        if (_outgoingPacketFlags & EntityItem::DIRTY_VELOCITY) {
            if (_body->isActive()) {
                _sentVelocity = bulletToGLM(_body->getLinearVelocity());
                _sentAngularVelocity = bulletToGLM(_body->getAngularVelocity());

                // if the speeds are very small we zero them out
                const float MINIMUM_EXTRAPOLATION_SPEED_SQUARED = 1.0e-4f; // 1cm/sec
                zeroSpeed = (glm::length2(_sentVelocity) < MINIMUM_EXTRAPOLATION_SPEED_SQUARED);
                if (zeroSpeed) {
                    _sentVelocity = glm::vec3(0.0f);
                }
                const float MINIMUM_EXTRAPOLATION_SPIN_SQUARED = 0.004f; // ~0.01 rotation/sec
                zeroSpin = glm::length2(_sentAngularVelocity) < MINIMUM_EXTRAPOLATION_SPIN_SQUARED;
                if (zeroSpin) {
                    _sentAngularVelocity = glm::vec3(0.0f);
                }

                _sentMoving = ! (zeroSpeed && zeroSpin);
            } else {
                _sentVelocity = _sentAngularVelocity = glm::vec3(0.0f);
                _sentMoving = false;
            }
            properties.setVelocity(_sentVelocity);
            _sentGravity = _entity->getGravity();
            properties.setGravity(_entity->getGravity());
            _sentAcceleration = _entity->getAcceleration();
            properties.setAcceleration(_sentAcceleration);
            properties.setAngularVelocity(_sentAngularVelocity);
        }

        auto nodeList = DependencyManager::get<NodeList>();
        QUuid myNodeID = nodeList->getSessionUUID();
        QUuid simulatorID = _entity->getSimulatorID();

        if (getShouldClaimSimulationOwnership()) {
            properties.setSimulatorID(myNodeID);
            setShouldClaimSimulationOwnership(false);
        } else if (simulatorID == myNodeID && zeroSpeed && zeroSpin) {
            // we are the simulator and the entity has stopped.  give up "simulator" status
            _entity->setSimulatorID(QUuid());
            properties.setSimulatorID(QUuid());
        } else if (simulatorID == myNodeID && !_body->isActive()) {
            // it's not active.  don't keep simulation ownership.
            _entity->setSimulatorID(QUuid());
            properties.setSimulatorID(QUuid());
        }

        // RELIABLE_SEND_HACK: count number of updates for entities at rest so we can stop sending them after some limit.
        if (_sentMoving) {
            _numNonMovingUpdates = 0;
        } else {
            _numNonMovingUpdates++;
        }
        if (_numNonMovingUpdates <= 1) {
            // we only update lastEdited when we're sending new physics data 
            // (i.e. NOT when we just simulate the positions forward, nor when we resend non-moving data)
            // NOTE: Andrew & Brad to discuss. Let's make sure we're using lastEdited, lastSimulated, and lastUpdated correctly
            quint64 lastSimulated = _entity->getLastSimulated();
            _entity->setLastEdited(lastSimulated);
            properties.setLastEdited(lastSimulated);

            #ifdef WANT_DEBUG
                quint64 now = usecTimestampNow();
                qCDebug(physics) << "EntityMotionState::sendUpdate()";
                qCDebug(physics) << "        EntityItemId:" << _entity->getEntityItemID()
                                 << "---------------------------------------------";
                qCDebug(physics) << "       lastSimulated:" << debugTime(lastSimulated, now);
            #endif //def WANT_DEBUG

        } else {
            properties.setLastEdited(_entity->getLastEdited());
        }

        if (EntityItem::getSendPhysicsUpdates()) {
            EntityItemID id(_entity->getID());
            EntityEditPacketSender* entityPacketSender = static_cast<EntityEditPacketSender*>(packetSender);
            #ifdef WANT_DEBUG
                qCDebug(physics) << "EntityMotionState::sendUpdate()... calling queueEditEntityMessage()...";
            #endif

            entityPacketSender->queueEditEntityMessage(PacketTypeEntityAddOrEdit, id, properties);
        } else {
            #ifdef WANT_DEBUG
                qCDebug(physics) << "EntityMotionState::sendUpdate()... NOT sending update as requested.";
            #endif
        }

        // The outgoing flags only itemized WHAT to send, not WHETHER to send, hence we always set them
        // to the full set.  These flags may be momentarily cleared by incoming external changes.  
        _outgoingPacketFlags = DIRTY_PHYSICS_FLAGS;
        _sentStep = step;
    }
}

uint32_t EntityMotionState::getIncomingDirtyFlags() const { 
    uint32_t dirtyFlags = _entity->getDirtyFlags(); 

    if (_body) {
        // we add DIRTY_MOTION_TYPE if the body's motion type disagrees with entity velocity settings
        int bodyFlags = _body->getCollisionFlags();
        bool isMoving = _entity->isMoving();
        if (((bodyFlags & btCollisionObject::CF_STATIC_OBJECT) && isMoving) ||
                (bodyFlags & btCollisionObject::CF_KINEMATIC_OBJECT && !isMoving)) {
            dirtyFlags |= EntityItem::DIRTY_MOTION_TYPE; 
        }
    }
    return dirtyFlags;
}
