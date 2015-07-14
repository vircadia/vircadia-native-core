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
#include <PhysicsCollisionGroups.h>

#include "BulletUtil.h"
#include "EntityMotionState.h"
#include "PhysicsEngine.h"
#include "PhysicsHelpers.h"
#include "PhysicsLogging.h"

#ifdef WANT_DEBUG_ENTITY_TREE_LOCKS
#include "EntityTree.h"
#endif

static const float ACCELERATION_EQUIVALENT_EPSILON_RATIO = 0.1f;
static const quint8 STEPS_TO_DECIDE_BALLISTIC = 4;

const uint32_t LOOPS_FOR_SIMULATION_ORPHAN = 50;
const quint64 USECS_BETWEEN_OWNERSHIP_BIDS = USECS_PER_SECOND / 5;

#ifdef WANT_DEBUG_ENTITY_TREE_LOCKS
bool EntityMotionState::entityTreeIsLocked() const {
    EntityTreeElement* element = _entity ? _entity->getElement() : nullptr;
    EntityTree* tree = element ? element->getTree() : nullptr;
    if (tree) {
        bool readSuccess = tree->tryLockForRead();
        if (readSuccess) {
            tree->unlock();
        }
        bool writeSuccess = tree->tryLockForWrite();
        if (writeSuccess) {
            tree->unlock();
        }
        if (readSuccess && writeSuccess) {
            return false;  // if we can take either kind of lock, there was no tree lock.
        }
        return true; // either read or write failed, so there is some lock in place.
    } else {
        return true;
    }
}
#else
bool entityTreeIsLocked() {
    return true;
}
#endif


EntityMotionState::EntityMotionState(btCollisionShape* shape, EntityItemPointer entity) :
    ObjectMotionState(shape),
    _entity(entity),
    _sentInactive(true),
    _lastStep(0),
    _serverPosition(0.0f),
    _serverRotation(),
    _serverVelocity(0.0f),
    _serverAngularVelocity(0.0f),
    _serverGravity(0.0f),
    _serverAcceleration(0.0f),
    _serverActionData(QByteArray()),
    _lastMeasureStep(0),
    _lastVelocity(glm::vec3(0.0f)),
    _measuredAcceleration(glm::vec3(0.0f)),
    _measuredDeltaTime(0.0f),
    _accelerationNearlyGravityCount(0),
    _nextOwnershipBid(0),
    _loopsWithoutOwner(0)
{
    _type = MOTIONSTATE_TYPE_ENTITY;
    assert(_entity != nullptr);
    assert(entityTreeIsLocked());
    setMass(_entity->computeMass());
}

EntityMotionState::~EntityMotionState() {
    // be sure to clear _entity before calling the destructor
    assert(!_entity);
}

void EntityMotionState::updateServerPhysicsVariables() {
    assert(entityTreeIsLocked());
    _serverPosition = _entity->getPosition();
    _serverRotation = _entity->getRotation();
    _serverVelocity = _entity->getVelocity();
    _serverAngularVelocity = _entity->getAngularVelocity();
    _serverAcceleration = _entity->getAcceleration();
    _serverActionData = _entity->getActionData();
}

// virtual
void EntityMotionState::handleEasyChanges(uint32_t flags, PhysicsEngine* engine) {
    assert(entityTreeIsLocked());
    updateServerPhysicsVariables();
    ObjectMotionState::handleEasyChanges(flags, engine);

    if (flags & EntityItem::DIRTY_SIMULATOR_ID) {
        _loopsWithoutOwner = 0;
        if (_entity->getSimulatorID().isNull()) {
            // simulation ownership is being removed
            // remove the ACTIVATION flag because this object is coming to rest
            // according to a remote simulation and we don't want to wake it up again
            flags &= ~EntityItem::DIRTY_PHYSICS_ACTIVATION;
            // hint to Bullet that the object is deactivating
            _body->setActivationState(WANTS_DEACTIVATION);
            _outgoingPriority = NO_PRORITY;
        } else  {
            _nextOwnershipBid = usecTimestampNow() + USECS_BETWEEN_OWNERSHIP_BIDS;
            if (engine->getSessionID() == _entity->getSimulatorID() || _entity->getSimulationPriority() > _outgoingPriority) {
                // we own the simulation or our priority looses to remote
                _outgoingPriority = NO_PRORITY;
            }
        }
    }
    if (flags & EntityItem::DIRTY_SIMULATOR_OWNERSHIP) {
        // (DIRTY_SIMULATOR_OWNERSHIP really means "we should bid for ownership with SCRIPT priority")
        // we're manipulating this object directly via script, so we artificially 
        // manipulate the logic to trigger an immediate bid for ownership
        setOutgoingPriority(SCRIPT_EDIT_SIMULATION_PRIORITY);
    }
    if ((flags & EntityItem::DIRTY_PHYSICS_ACTIVATION) && !_body->isActive()) {
        _body->activate();
    }
}


// virtual
void EntityMotionState::handleHardAndEasyChanges(uint32_t flags, PhysicsEngine* engine) {
    updateServerPhysicsVariables();
    ObjectMotionState::handleHardAndEasyChanges(flags, engine);
}

void EntityMotionState::clearObjectBackPointer() {
    ObjectMotionState::clearObjectBackPointer();
    _entity = nullptr;
}

MotionType EntityMotionState::computeObjectMotionType() const {
    if (!_entity) {
        return MOTION_TYPE_STATIC;
    }
    assert(entityTreeIsLocked());
    if (_entity->getCollisionsWillMove()) {
        return MOTION_TYPE_DYNAMIC;
    }
    return _entity->isMoving() ?  MOTION_TYPE_KINEMATIC : MOTION_TYPE_STATIC;
}

bool EntityMotionState::isMoving() const {
    assert(entityTreeIsLocked());
    return _entity && _entity->isMoving();
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
    assert(entityTreeIsLocked());
    if (_motionType == MOTION_TYPE_KINEMATIC) {
        // This is physical kinematic motion which steps strictly by the subframe count
        // of the physics simulation.
        uint32_t thisStep = ObjectMotionState::getWorldSimulationStep();
        float dt = (thisStep - _lastKinematicStep) * PHYSICS_ENGINE_FIXED_SUBSTEP;
        _entity->simulateKinematicMotion(dt);

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
    assert(entityTreeIsLocked());
    measureBodyAcceleration();
    _entity->setPosition(bulletToGLM(worldTrans.getOrigin()) + ObjectMotionState::getWorldOffset());
    _entity->setRotation(bulletToGLM(worldTrans.getRotation()));

    _entity->setVelocity(getBodyLinearVelocity());
    _entity->setAngularVelocity(getBodyAngularVelocity());

    _entity->setLastSimulated(usecTimestampNow());

    if (_entity->getSimulatorID().isNull()) {
        _loopsWithoutOwner++;

        if (_loopsWithoutOwner > LOOPS_FOR_SIMULATION_ORPHAN && usecTimestampNow() > _nextOwnershipBid) {
            //qDebug() << "Warning -- claiming something I saw moving." << getName();
            setOutgoingPriority(VOLUNTEER_SIMULATION_PRIORITY);
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
btCollisionShape* EntityMotionState::computeNewShape() {
    if (_entity) {
        ShapeInfo shapeInfo;
        assert(entityTreeIsLocked());
        _entity->computeShapeInfo(shapeInfo);
        return getShapeManager()->getShape(shapeInfo);
    }
    return nullptr;
}

bool EntityMotionState::isCandidateForOwnership(const QUuid& sessionID) const {
    if (!_body || !_entity) {
        return false;
    }
    assert(entityTreeIsLocked());
    return _outgoingPriority != NO_PRORITY || sessionID == _entity->getSimulatorID();
}

bool EntityMotionState::remoteSimulationOutOfSync(uint32_t simulationStep) {
    assert(_body);
    // if we've never checked before, our _lastStep will be 0, and we need to initialize our state
    if (_lastStep == 0) {
        btTransform xform = _body->getWorldTransform();
        _serverPosition = bulletToGLM(xform.getOrigin());
        _serverRotation = bulletToGLM(xform.getRotation());
        _serverVelocity = getBodyLinearVelocity();
        _serverAngularVelocity = bulletToGLM(_body->getAngularVelocity());
        _lastStep = simulationStep;
        _serverActionData = _entity->getActionData();
        _sentInactive = true;
        return false;
    }

    #ifdef WANT_DEBUG
    glm::vec3 wasPosition = _serverPosition;
    glm::quat wasRotation = _serverRotation;
    glm::vec3 wasAngularVelocity = _serverAngularVelocity;
    #endif

    int numSteps = simulationStep - _lastStep;
    float dt = (float)(numSteps) * PHYSICS_ENGINE_FIXED_SUBSTEP;

    const float INACTIVE_UPDATE_PERIOD = 0.5f;
    if (_sentInactive) {
        // we resend the inactive update every INACTIVE_UPDATE_PERIOD
        // until it is removed from the outgoing updates
        // (which happens when we don't own the simulation and it isn't touching our simulation)
        return (dt > INACTIVE_UPDATE_PERIOD);
    }

    bool isActive = _body->isActive();
    if (!isActive) {
        // object has gone inactive but our last send was moving --> send non-moving update immediately
        return true;
    }

    _lastStep = simulationStep;
    if (glm::length2(_serverVelocity) > 0.0f) {
        _serverVelocity += _serverAcceleration * dt;
        _serverVelocity *= powf(1.0f - _body->getLinearDamping(), dt);
        _serverPosition += dt * _serverVelocity;
    }

    if (_serverActionData != _entity->getActionData()) {
        setOutgoingPriority(SCRIPT_EDIT_SIMULATION_PRIORITY);
        return true;
    }

    // Else we measure the error between current and extrapolated transform (according to expected behavior
    // of remote EntitySimulation) and return true if the error is significant.

    // NOTE: math is done in the simulation-frame, which is NOT necessarily the same as the world-frame
    // due to _worldOffset.
    // TODO: compensate for _worldOffset offset here

    // compute position error

    btTransform worldTrans = _body->getWorldTransform();
    glm::vec3 position = bulletToGLM(worldTrans.getOrigin());

    float dx2 = glm::distance2(position, _serverPosition);

    const float MAX_POSITION_ERROR_SQUARED = 0.000004f; //  Sqrt() - corresponds to 2 millimeters
    if (dx2 > MAX_POSITION_ERROR_SQUARED) {

        #ifdef WANT_DEBUG
            qCDebug(physics) << ".... (dx2 > MAX_POSITION_ERROR_SQUARED) ....";
            qCDebug(physics) << "wasPosition:" << wasPosition;
            qCDebug(physics) << "bullet position:" << position;
            qCDebug(physics) << "_serverPosition:" << _serverPosition;
            qCDebug(physics) << "dx2:" << dx2;
        #endif

        return true;
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
    glm::quat actualRotation = bulletToGLM(worldTrans.getRotation());

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

bool EntityMotionState::shouldSendUpdate(uint32_t simulationStep, const QUuid& sessionID) {
    // NOTE: we expect _entity and _body to be valid in this context, since shouldSendUpdate() is only called
    // after doesNotNeedToSendUpdate() returns false and that call should return 'true' if _entity or _body are NULL.
    assert(_entity);
    assert(_body);
    assert(entityTreeIsLocked());

    if (_entity->getSimulatorID() != sessionID) {
        // we don't own the simulation, but maybe we should...
        if (_outgoingPriority != NO_PRORITY) {
            if (_outgoingPriority < _entity->getSimulationPriority()) {
                // our priority looses to remote, so we don't bother to bid
                _outgoingPriority = NO_PRORITY;
                return false;
            }
            return usecTimestampNow() > _nextOwnershipBid;
        }
        return false;
    }

    return remoteSimulationOutOfSync(simulationStep);
}

void EntityMotionState::sendUpdate(OctreeEditPacketSender* packetSender, const QUuid& sessionID, uint32_t step) {
    assert(_entity);
    assert(entityTreeIsLocked());

    bool active = _body->isActive();
    if (!active) {
        // make sure all derivatives are zero
        glm::vec3 zero(0.0f);
        _entity->setVelocity(zero);
        _entity->setAngularVelocity(zero);
        _entity->setAcceleration(zero);
        _sentInactive = true;
    } else {
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

        const float DYNAMIC_LINEAR_VELOCITY_THRESHOLD = 0.05f;  // 5 cm/sec
        const float DYNAMIC_ANGULAR_VELOCITY_THRESHOLD = 0.087266f;  // ~5 deg/sec

        bool movingSlowlyLinear =
            glm::length2(_entity->getVelocity()) < (DYNAMIC_LINEAR_VELOCITY_THRESHOLD * DYNAMIC_LINEAR_VELOCITY_THRESHOLD);
        bool movingSlowlyAngular = glm::length2(_entity->getAngularVelocity()) <
                (DYNAMIC_ANGULAR_VELOCITY_THRESHOLD * DYNAMIC_ANGULAR_VELOCITY_THRESHOLD);
        bool movingSlowly = movingSlowlyLinear && movingSlowlyAngular && _entity->getAcceleration() == glm::vec3(0.0f);

        if (movingSlowly) {
            // velocities might not be zero, but we'll fake them as such, which will hopefully help convince
            // other simulating observers to deactivate their own copies
            glm::vec3 zero(0.0f);
            _entity->setVelocity(zero);
            _entity->setAngularVelocity(zero);
        }
        _sentInactive = false;
    }

    // remember properties for local server prediction
    _serverPosition = _entity->getPosition();
    _serverRotation = _entity->getRotation();
    _serverVelocity = _entity->getVelocity();
    _serverAcceleration = _entity->getAcceleration();
    _serverAngularVelocity = _entity->getAngularVelocity();
    _serverActionData = _entity->getActionData();

    EntityItemProperties properties;

    // explicitly set the properties that changed so that they will be packed
    properties.setPosition(_serverPosition);
    properties.setRotation(_serverRotation);
    properties.setVelocity(_serverVelocity);
    properties.setAcceleration(_serverAcceleration);
    properties.setAngularVelocity(_serverAngularVelocity);
    properties.setActionData(_serverActionData);

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

    if (sessionID == _entity->getSimulatorID()) {
        // we think we own the simulation
        if (!active) {
            // we own the simulation but the entity has stopped, so we tell the server that we're clearing simulatorID
            // but we remember that we do still own it...  and rely on the server to tell us that we don't
            properties.clearSimulationOwner();
            _outgoingPriority = NO_PRORITY;
        }
        // else the ownership is not changing so we don't bother to pack it
    } else {
        // we don't own the simulation for this entity yet, but we're sending a bid for it
        properties.setSimulationOwner(sessionID, glm::max<quint8>(_outgoingPriority, VOLUNTEER_SIMULATION_PRIORITY));
        _nextOwnershipBid = now + USECS_BETWEEN_OWNERSHIP_BIDS;
    }

    if (EntityItem::getSendPhysicsUpdates()) {
        EntityItemID id(_entity->getID());
        EntityEditPacketSender* entityPacketSender = static_cast<EntityEditPacketSender*>(packetSender);
        #ifdef WANT_DEBUG
            qCDebug(physics) << "EntityMotionState::sendUpdate()... calling queueEditEntityMessage()...";
        #endif

        entityPacketSender->queueEditEntityMessage(PacketType::EntityEdit, id, properties);
        _entity->setLastBroadcast(usecTimestampNow());
    } else {
        #ifdef WANT_DEBUG
            qCDebug(physics) << "EntityMotionState::sendUpdate()... NOT sending update as requested.";
        #endif
    }

    _lastStep = step;
}

uint32_t EntityMotionState::getAndClearIncomingDirtyFlags() {
    assert(entityTreeIsLocked());
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
quint8 EntityMotionState::getSimulationPriority() const {
    if (_entity) {
        return _entity->getSimulationPriority();
    }
    return NO_PRORITY;
}

// virtual
QUuid EntityMotionState::getSimulatorID() const {
    if (_entity) {
        assert(entityTreeIsLocked());
        return _entity->getSimulatorID();
    }
    return QUuid();
}

// virtual
void EntityMotionState::bump(quint8 priority) {
    if (_entity) {
        setOutgoingPriority(glm::max(VOLUNTEER_SIMULATION_PRIORITY, --priority));
    }
}

void EntityMotionState::resetMeasuredBodyAcceleration() {
    _lastMeasureStep = ObjectMotionState::getWorldSimulationStep();
    if (_body) {
        _lastVelocity = getBodyLinearVelocity();
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
        _measuredDeltaTime = dt;

        // Note: the integration equation for velocity uses damping:   v1 = (v0 + a * dt) * (1 - D)^dt
        // hence the equation for acceleration is: a = (v1 / (1 - D)^dt - v0) / dt
        glm::vec3 velocity = getBodyLinearVelocity();
        _measuredAcceleration = (velocity / powf(1.0f - _body->getLinearDamping(), dt) - _lastVelocity) * invDt;
        _lastVelocity = velocity;
        if (numSubsteps > PHYSICS_ENGINE_MAX_NUM_SUBSTEPS) {
            _loopsWithoutOwner = 0;
            _lastStep = ObjectMotionState::getWorldSimulationStep();
            _sentInactive = false;
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
void EntityMotionState::setMotionType(MotionType motionType) {
    ObjectMotionState::setMotionType(motionType);
    resetMeasuredBodyAcceleration();
}


// virtual
QString EntityMotionState::getName() {
    if (_entity) {
        assert(entityTreeIsLocked());
        return _entity->getName();
    }
    return "";
}

// virtual
int16_t EntityMotionState::computeCollisionGroup() {
    switch (computeObjectMotionType()){
        case MOTION_TYPE_STATIC:
            return COLLISION_GROUP_STATIC;
        case MOTION_TYPE_KINEMATIC:
            return COLLISION_GROUP_KINEMATIC;
        default:
            break;
    }
    return COLLISION_GROUP_DEFAULT;
}

void EntityMotionState::setOutgoingPriority(quint8 priority) {
    _outgoingPriority = glm::max<quint8>(_outgoingPriority, priority);
}
