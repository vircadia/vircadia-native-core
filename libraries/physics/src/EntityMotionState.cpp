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
    // be sure to clear _entity before calling the destructor
    assert(!_entity);
}

void EntityMotionState::updateServerPhysicsVariables(uint32_t flags) {
    /*
    if (flags & EntityItem::DIRTY_POSITION) {
        _sentPosition = _entity->getPosition();
    }
    if (flags & EntityItem::DIRTY_ROTATION) {
        _sentRotation = _entity->getRotation();
    }
    if (flags & EntityItem::DIRTY_LINEAR_VELOCITY) {
        _sentVelocity = _entity->getVelocity();
    }
    if (flags & EntityItem::DIRTY_ANGULAR_VELOCITY) {
        _sentAngularVelocity = _entity->getAngularVelocity();
    }
    */
}

// virtual
void EntityMotionState::handleEasyChanges(uint32_t flags) {
    updateServerPhysicsVariables(flags);
    ObjectMotionState::handleEasyChanges(flags);
}


// virtual
void EntityMotionState::handleHardAndEasyChanges(uint32_t flags, PhysicsEngine* engine) {
    updateServerPhysicsVariables(flags);
    ObjectMotionState::handleHardAndEasyChanges(flags, engine);
}

void EntityMotionState::clearEntity() {
    _entity = nullptr;
}

MotionType EntityMotionState::computeObjectMotionType() const {
    if (!_entity) {
        return MOTION_TYPE_STATIC;
    }
    if (_entity->getCollisionsWillMove()) {
        return MOTION_TYPE_DYNAMIC;
    }
    return _entity->isMoving() ?  MOTION_TYPE_KINEMATIC : MOTION_TYPE_STATIC;
}

bool EntityMotionState::isMoving() const {
    return _entity && _entity->isMoving();
}

bool EntityMotionState::isMovingVsServer() const {
    // auto alignmentDot = glm::abs(glm::dot(_sentRotation, _entity->getRotation()));
    // if (glm::distance(_sentPosition, _entity->getPosition()) > IGNORE_POSITION_DELTA ||
    //     alignmentDot < IGNORE_ALIGNMENT_DOT) {
    //     return true;
    // }
    // return false;

    if (glm::length(_entity->getVelocity()) > IGNORE_LINEAR_VELOCITY_DELTA) {
        return true;
    }
    if (glm::length(_entity->getAngularVelocity()) > IGNORE_ANGULAR_VELOCITY_DELTA) {
        return true;
    }
    return false;
}

// This callback is invoked by the physics simulation in two cases:
// (1) when the RigidBody is first added to the world
//     (irregardless of MotionType: STATIC, DYNAMIC, or KINEMATIC)
// (2) at the beginning of each simulation step for KINEMATIC RigidBody's --
//     it is an opportunity for outside code to update the object's simulation position
void EntityMotionState::getWorldTransform(btTransform& worldTrans) const {
    if (!_entity) {
        return;
    }
    if (_motionType == MOTION_TYPE_KINEMATIC) {
        // This is physical kinematic motion which steps strictly by the subframe count
        // of the physics simulation.
        uint32_t thisStep = ObjectMotionState::getWorldSimulationStep();
        float dt = (thisStep - _lastKinematicStep) * PHYSICS_ENGINE_FIXED_SUBSTEP;
        _entity->simulateKinematicMotion(dt);
        _entity->setLastSimulated(usecTimestampNow());

        // bypass const-ness so we can remember the step
        const_cast<EntityMotionState*>(this)->_lastKinematicStep = thisStep;
    }
    worldTrans.setOrigin(glmToBullet(getObjectPosition()));
    worldTrans.setRotation(glmToBullet(_entity->getRotation()));
}

// This callback is invoked by the physics simulation at the end of each simulation step...
// iff the corresponding RigidBody is DYNAMIC and has moved.
void EntityMotionState::setWorldTransform(const btTransform& worldTrans) {
    if (!_entity) {
        return;
    }
    measureBodyAcceleration();
    _entity->setPosition(bulletToGLM(worldTrans.getOrigin()) + ObjectMotionState::getWorldOffset());
    _entity->setRotation(bulletToGLM(worldTrans.getRotation()));

    _entity->setVelocity(getBodyLinearVelocity());
    _entity->setAngularVelocity(getBodyAngularVelocity());

    _entity->setLastSimulated(usecTimestampNow());

    // if (_entity->getSimulatorID().isNull() && isMoving()) {
    if (_entity->getSimulatorID().isNull() && isMovingVsServer()) {
        // if object is moving and has no owner, attempt to claim simulation ownership.
        _movingStepsWithoutSimulationOwner++;
    } else {
        _movingStepsWithoutSimulationOwner = 0;
    }

    if (_movingStepsWithoutSimulationOwner > 50) { // XXX maybe meters from our characterController ?
        qDebug() << "XXX XXX XXX -- claiming something I saw moving";
        setShouldClaimSimulationOwnership(true);
    }

    #ifdef WANT_DEBUG
        quint64 now = usecTimestampNow();
        qCDebug(physics) << "EntityMotionState::setWorldTransform()... changed entity:" << _entity->getEntityItemID();
        qCDebug(physics) << "       last edited:" << _entity->getLastEdited() << formatUsecTime(now - _entity->getLastEdited()) << "ago";
        qCDebug(physics) << "    last simulated:" << _entity->getLastSimulated() << formatUsecTime(now - _entity->getLastSimulated()) << "ago";
        qCDebug(physics) << "      last updated:" << _entity->getLastUpdated() << formatUsecTime(now - _entity->getLastUpdated()) << "ago";
    #endif
}

void EntityMotionState::computeObjectShapeInfo(ShapeInfo& shapeInfo) {
    if (_entity) {
        _entity->computeShapeInfo(shapeInfo);
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
    if (!_entity || !remoteSimulationOutOfSync(simulationFrame)) {
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
    if (!_entity || !_entity->isKnownID()) {
        return; // never update entities that are unknown
    }

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

    btTransform worldTrans = _body->getWorldTransform();
    _sentPosition = bulletToGLM(worldTrans.getOrigin());
    properties.setPosition(_sentPosition + ObjectMotionState::getWorldOffset());
  
    _sentRotation = bulletToGLM(worldTrans.getRotation());
    properties.setRotation(_sentRotation);

    bool zeroSpeed = true;
    bool zeroSpin = true;

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
        _entity->setLastBroadcast(usecTimestampNow());
    } else {
        #ifdef WANT_DEBUG
            qCDebug(physics) << "EntityMotionState::sendUpdate()... NOT sending update as requested.";
        #endif
    }

    _sentStep = step;
}

uint32_t EntityMotionState::getAndClearIncomingDirtyFlags() const { 
    uint32_t dirtyFlags = 0;
    if (_body && _entity) {
        dirtyFlags = _entity->getDirtyFlags(); 
        _entity->clearDirtyFlags();
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


// virtual
QUuid EntityMotionState::getSimulatorID() const {
    if (_entity) {
        return _entity->getSimulatorID();
    }
    return QUuid();
}


// virtual
void EntityMotionState::bump() {
    setShouldClaimSimulationOwnership(true);
}

void EntityMotionState::resetMeasuredBodyAcceleration() {
    _lastMeasureStep = ObjectMotionState::getWorldSimulationStep();
    if (_body) {
        _lastVelocity = bulletToGLM(_body->getLinearVelocity());                                                        
    } else {
        _lastVelocity = glm::vec3(0.0f);
    }
    _measuredAcceleration = glm::vec3(0.0f);
} 

void EntityMotionState::measureBodyAcceleration() {
    // try to manually measure the true acceleration of the object
    uint32_t thisStep = ObjectMotionState::getWorldSimulationStep();
    uint32_t numSubsteps = thisStep - _lastMeasureStep;
    if (numSubsteps > 0) {
        float dt = ((float)numSubsteps * PHYSICS_ENGINE_FIXED_SUBSTEP);
        float invDt = 1.0f / dt;
        _lastMeasureStep = thisStep;

        // Note: the integration equation for velocity uses damping:   v1 = (v0 + a * dt) * (1 - D)^dt
        // hence the equation for acceleration is: a = (v1 / (1 - D)^dt - v0) / dt
        glm::vec3 velocity = bulletToGLM(_body->getLinearVelocity());
        _measuredAcceleration = (velocity / powf(1.0f - _body->getLinearDamping(), dt) - _lastVelocity) * invDt;
        _lastVelocity = velocity;
    }
}

// virtual 
void EntityMotionState::setMotionType(MotionType motionType) {
    ObjectMotionState::setMotionType(motionType);
    resetMeasuredBodyAcceleration();
}
