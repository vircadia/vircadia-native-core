//
//  PhysicsEngine.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <AABox.h> 

#include "PhysicsEngine.h"
#include "ShapeInfoUtil.h"
#include "PhysicsHelpers.h"
#include "ThreadSafeDynamicsWorld.h"
#include "PhysicsLogging.h"

static uint32_t _numSubsteps;

// static
uint32_t PhysicsEngine::getNumSubsteps() {
    return _numSubsteps;
}

PhysicsEngine::PhysicsEngine(const glm::vec3& offset) :
    _originOffset(offset),
    _characterController(NULL) {
}

PhysicsEngine::~PhysicsEngine() {
    if (_characterController) {
        _characterController->setDynamicsWorld(NULL);
    }
    // TODO: delete engine components... if we ever plan to create more than one instance
    delete _collisionConfig;
    delete _collisionDispatcher;
    delete _broadphaseFilter;
    delete _constraintSolver;
    delete _dynamicsWorld;
    delete _ghostPairCallback;
}

// begin EntitySimulation overrides
void PhysicsEngine::updateEntitiesInternal(const quint64& now) {
    // no need to send updates unless the physics simulation has actually stepped
    if (_lastNumSubstepsAtUpdateInternal != _numSubsteps) {
        _lastNumSubstepsAtUpdateInternal = _numSubsteps;
        // NOTE: the grand order of operations is:
        // (1) relay incoming changes
        // (2) step simulation
        // (3) synchronize outgoing motion states
        // (4) send outgoing packets
    
        // this is step (4)
        QSet<ObjectMotionState*>::iterator stateItr = _outgoingPackets.begin();
        while (stateItr != _outgoingPackets.end()) {
            ObjectMotionState* state = *stateItr;
            if (state->doesNotNeedToSendUpdate()) {
                stateItr = _outgoingPackets.erase(stateItr);
            } else if (state->shouldSendUpdate(_numSubsteps)) {
                state->sendUpdate(_entityPacketSender, _numSubsteps);
                ++stateItr;
            } else {
                ++stateItr;
            }
        }
    }
}

void PhysicsEngine::addEntityInternal(EntityItem* entity) {
    assert(entity);
    void* physicsInfo = entity->getPhysicsInfo();
    if (!physicsInfo) {
        if (entity->isReadyToComputeShape()) {
            ShapeInfo shapeInfo;
            entity->computeShapeInfo(shapeInfo);
            btCollisionShape* shape = _shapeManager.getShape(shapeInfo);
            if (shape) {
                EntityMotionState* motionState = new EntityMotionState(entity);
                entity->setPhysicsInfo(static_cast<void*>(motionState));
                _entityMotionStates.insert(motionState);
                addObject(shapeInfo, shape, motionState);
            } else if (entity->isMoving()) {
                EntityMotionState* motionState = new EntityMotionState(entity);
                entity->setPhysicsInfo(static_cast<void*>(motionState));
                _entityMotionStates.insert(motionState);

                motionState->setKinematic(true, _numSubsteps);
                _nonPhysicalKinematicObjects.insert(motionState);
                // We failed to add the entity to the simulation.  Probably because we couldn't create a shape for it.
                //qCDebug(physics) << "failed to add entity " << entity->getEntityItemID() << " to physics engine";
            }
        }
    }
}

void PhysicsEngine::removeEntityInternal(EntityItem* entity) {
    assert(entity);
    void* physicsInfo = entity->getPhysicsInfo();
    if (physicsInfo) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(physicsInfo);
        if (motionState->getRigidBody()) {
            removeObjectFromBullet(motionState);
        } else {
            // only need to hunt in this list when there is no RigidBody
            _nonPhysicalKinematicObjects.remove(motionState);
        }
        _entityMotionStates.remove(motionState);
        _incomingChanges.remove(motionState);
        _outgoingPackets.remove(motionState);
        // NOTE: EntityMotionState dtor will remove its backpointer from EntityItem
        delete motionState;
    }
}

void PhysicsEngine::entityChangedInternal(EntityItem* entity) {
    // queue incoming changes: from external sources (script, EntityServer, etc) to physics engine
    assert(entity);
    void* physicsInfo = entity->getPhysicsInfo();
    if (physicsInfo) {
        if ((entity->getDirtyFlags() & (HARD_DIRTY_PHYSICS_FLAGS | EASY_DIRTY_PHYSICS_FLAGS)) > 0) {
            ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
            _incomingChanges.insert(motionState);
        }
    } else {
        // try to add this entity again (maybe something changed such that it will work this time)
        addEntity(entity);
    }
}

void PhysicsEngine::sortEntitiesThatMovedInternal() {
    // entities that have been simulated forward (hence in the _entitiesToBeSorted list) 
    // also need to be put in the outgoingPackets list
    QSet<EntityItem*>::iterator entityItr = _entitiesToBeSorted.begin();
    while (entityItr != _entitiesToBeSorted.end()) {
        EntityItem* entity = *entityItr;
        void* physicsInfo = entity->getPhysicsInfo();
        assert(physicsInfo);
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
        _outgoingPackets.insert(motionState);
        ++entityItr;
    }
}

void PhysicsEngine::clearEntitiesInternal() {
    // For now we assume this would only be called on shutdown in which case we can just let the memory get lost.
    QSet<EntityMotionState*>::const_iterator stateItr = _entityMotionStates.begin();
    for (stateItr = _entityMotionStates.begin(); stateItr != _entityMotionStates.end(); ++stateItr) {
        removeObjectFromBullet(*stateItr);
        delete (*stateItr);
    }
    _entityMotionStates.clear();
    _nonPhysicalKinematicObjects.clear();
    _incomingChanges.clear();
    _outgoingPackets.clear();
}
// end EntitySimulation overrides

void PhysicsEngine::relayIncomingChangesToSimulation() {
    BT_PROFILE("incomingChanges");
    // process incoming changes
    QSet<ObjectMotionState*>::iterator stateItr = _incomingChanges.begin();
    while (stateItr != _incomingChanges.end()) {
        ObjectMotionState* motionState = *stateItr;
        ++stateItr;
        uint32_t flags = motionState->getIncomingDirtyFlags() & DIRTY_PHYSICS_FLAGS;

        bool removeMotionState = false;
        btRigidBody* body = motionState->getRigidBody();
        if (body) {
            if (flags & HARD_DIRTY_PHYSICS_FLAGS) {
                // a HARD update requires the body be pulled out of physics engine, changed, then reinserted
                // but it also handles all EASY changes
                bool success = updateBodyHard(body, motionState, flags);
                if (!success) {
                    // NOTE: since updateBodyHard() failed we know that motionState has been removed 
                    // from simulation and body has been deleted.  Depending on what else has changed
                    // we might need to remove motionState altogether...
                    if (flags & EntityItem::DIRTY_VELOCITY) {
                        motionState->updateKinematicState(_numSubsteps);
                        if (motionState->isKinematic()) {
                            // all is NOT lost, we still need to move this object around kinematically
                            _nonPhysicalKinematicObjects.insert(motionState);
                        } else {
                            // no need to keep motionState around
                            removeMotionState = true;
                        }
                    } else {
                         // no need to keep motionState around
                         removeMotionState = true;
                    }
                }
            } else if (flags) {
                // an EASY update does NOT require that the body be pulled out of physics engine
                // hence the MotionState has all the knowledge and authority to perform the update.
                motionState->updateBodyEasy(flags, _numSubsteps);
            }
            if (flags & (EntityItem::DIRTY_POSITION | EntityItem::DIRTY_VELOCITY)) {
                motionState->resetMeasuredBodyAcceleration();
            }
        } else {
            // the only way we should ever get here (motionState exists but no body) is when the object
            // is undergoing non-physical kinematic motion.
            assert(_nonPhysicalKinematicObjects.contains(motionState));

            // it is possible that the changes are such that the object can now be added to the physical simulation
            if (flags & EntityItem::DIRTY_SHAPE) {
                ShapeInfo shapeInfo;
                motionState->computeObjectShapeInfo(shapeInfo);
                btCollisionShape* shape = _shapeManager.getShape(shapeInfo);
                if (shape) {
                    addObject(shapeInfo, shape, motionState);
                    _nonPhysicalKinematicObjects.remove(motionState);
                } else if (flags & EntityItem::DIRTY_VELOCITY) {
                    // although we couldn't add the object to the simulation, might need to update kinematic motion...
                    motionState->updateKinematicState(_numSubsteps);
                    if (!motionState->isKinematic()) {
                        _nonPhysicalKinematicObjects.remove(motionState);
                        removeMotionState = true;
                    }
                }
            } else if (flags & EntityItem::DIRTY_VELOCITY) {
                // although we still can't add to physics simulation, might need to update kinematic motion...
                motionState->updateKinematicState(_numSubsteps);
                if (!motionState->isKinematic()) {
                    _nonPhysicalKinematicObjects.remove(motionState);
                    removeMotionState = true;
                }
            }
        }
        if (removeMotionState) {
            // if we get here then there is no need to keep this motionState around (no physics or kinematics)
            _outgoingPackets.remove(motionState);
            if (motionState->getType() == MOTION_STATE_TYPE_ENTITY) {
                _entityMotionStates.remove(static_cast<EntityMotionState*>(motionState));
            }
            // NOTE: motionState will clean up its own backpointers in the Object
            delete motionState;
            continue;
        }

        // NOTE: the grand order of operations is:
        // (1) relay incoming changes
        // (2) step simulation
        // (3) synchronize outgoing motion states
        // (4) send outgoing packets
        //
        // We're in the middle of step (1) hence incoming changes should trump corresponding 
        // outgoing changes at this point.
        motionState->clearOutgoingPacketFlags(flags); // clear outgoing flags that were trumped
        motionState->clearIncomingDirtyFlags(flags);  // clear incoming flags that were processed
    }
    _incomingChanges.clear();
}

void PhysicsEngine::removeContacts(ObjectMotionState* motionState) {
    // trigger events for new/existing/old contacts
    ContactMap::iterator contactItr = _contactMap.begin();
    while (contactItr != _contactMap.end()) {
        if (contactItr->first._a == motionState || contactItr->first._b == motionState) {
            ContactMap::iterator iterToDelete = contactItr;
            ++contactItr;
            _contactMap.erase(iterToDelete);
        } else {
            ++contactItr;
        }
    }
}

// virtual
void PhysicsEngine::init(EntityEditPacketSender* packetSender) {
    // _entityTree should be set prior to the init() call
    assert(_entityTree);

    if (!_dynamicsWorld) {
        _collisionConfig = new btDefaultCollisionConfiguration();
        _collisionDispatcher = new btCollisionDispatcher(_collisionConfig);
        _broadphaseFilter = new btDbvtBroadphase();
        _constraintSolver = new btSequentialImpulseConstraintSolver;
        _dynamicsWorld = new ThreadSafeDynamicsWorld(_collisionDispatcher, _broadphaseFilter, _constraintSolver, _collisionConfig);

        _ghostPairCallback = new btGhostPairCallback();
        _dynamicsWorld->getPairCache()->setInternalGhostPairCallback(_ghostPairCallback);

        // default gravity of the world is zero, so each object must specify its own gravity
        // TODO: set up gravity zones
        _dynamicsWorld->setGravity(btVector3(0.0f, 0.0f, 0.0f));
    }

    assert(packetSender);
    _entityPacketSender = packetSender;
    EntityMotionState::setOutgoingEntityList(&_entitiesToBeSorted);
}

void PhysicsEngine::stepSimulation() {
    lock();
    CProfileManager::Reset();
    BT_PROFILE("stepSimulation");
    // NOTE: the grand order of operations is:
    // (1) pull incoming changes
    // (2) step simulation
    // (3) synchronize outgoing motion states
    // (4) send outgoing packets

    // This is step (1) pull incoming changes
    relayIncomingChangesToSimulation();

    const int MAX_NUM_SUBSTEPS = 4;
    const float MAX_TIMESTEP = (float)MAX_NUM_SUBSTEPS * PHYSICS_ENGINE_FIXED_SUBSTEP;
    float dt = 1.0e-6f * (float)(_clock.getTimeMicroseconds());
    _clock.reset();
    float timeStep = btMin(dt, MAX_TIMESTEP);

    // TODO: move character->preSimulation() into relayIncomingChanges
    if (_characterController) {
        if (_characterController->needsRemoval()) {
            _characterController->setDynamicsWorld(NULL);
        }
        _characterController->updateShapeIfNecessary();
        if (_characterController->needsAddition()) {
            _characterController->setDynamicsWorld(_dynamicsWorld);
        }
        _characterController->preSimulation(timeStep);
    }

    // This is step (2) step simulation
    int numSubsteps = _dynamicsWorld->stepSimulation(timeStep, MAX_NUM_SUBSTEPS, PHYSICS_ENGINE_FIXED_SUBSTEP);
    _numSubsteps += (uint32_t)numSubsteps;
    stepNonPhysicalKinematics(usecTimestampNow());
    unlock();

    // TODO: make all of this harvest stuff into one function: relayOutgoingChanges()
    if (numSubsteps > 0) {
        BT_PROFILE("postSimulation");
        // This is step (3) which is done outside of stepSimulation() so we can lock _entityTree.
        //
        // Unfortunately we have to unlock the simulation (above) before we try to lock the _entityTree
        // to avoid deadlock -- the _entityTree may try to lock its EntitySimulation (from which this 
        // PhysicsEngine derives) when updating/adding/deleting entities so we need to wait for our own
        // lock on the tree before we re-lock ourselves.
        //
        // TODO: untangle these lock sequences.
        ObjectMotionState::setWorldSimulationStep(_numSubsteps);
        _entityTree->lockForWrite();
        lock();
        _dynamicsWorld->synchronizeMotionStates();

        if (_characterController) {
            _characterController->postSimulation();
        }

        computeCollisionEvents();

        unlock();
        _entityTree->unlock();
    }
}

void PhysicsEngine::stepNonPhysicalKinematics(const quint64& now) {
    BT_PROFILE("nonPhysicalKinematics");
    QSet<ObjectMotionState*>::iterator stateItr = _nonPhysicalKinematicObjects.begin();
    // TODO?: need to occasionally scan for stopped non-physical kinematics objects
    while (stateItr != _nonPhysicalKinematicObjects.end()) {
        ObjectMotionState* motionState = *stateItr;
        motionState->stepKinematicSimulation(now);
        ++stateItr;
    }
}


void PhysicsEngine::doOwnershipInfection(const btCollisionObject* objectA, const btCollisionObject* objectB) {
    assert(objectA);
    assert(objectB);

    auto nodeList = DependencyManager::get<NodeList>();
    QUuid myNodeID = nodeList->getSessionUUID();

    if (myNodeID.isNull()) {
        return;
    }

    const btCollisionObject* characterCollisionObject =
        _characterController ? _characterController->getCollisionObject() : NULL;

    ObjectMotionState* a = static_cast<ObjectMotionState*>(objectA->getUserPointer());
    ObjectMotionState* b = static_cast<ObjectMotionState*>(objectB->getUserPointer());
    EntityItem* entityA = a ? a->getEntity() : NULL;
    EntityItem* entityB = b ? b->getEntity() : NULL;
    bool aIsDynamic = entityA && !objectA->isStaticOrKinematicObject();
    bool bIsDynamic = entityB && !objectB->isStaticOrKinematicObject();

    // collisions cause infectious spread of simulation-ownership.  we also attempt to take
    // ownership of anything that collides with our avatar.
    if ((aIsDynamic && (entityA->getSimulatorID() == myNodeID)) ||
        // (a && a->getShouldClaimSimulationOwnership()) ||
        (objectA == characterCollisionObject)) {
        if (bIsDynamic) {
            b->setShouldClaimSimulationOwnership(true);
        }
    } else if ((bIsDynamic && (entityB->getSimulatorID() == myNodeID)) ||
        // (b && b->getShouldClaimSimulationOwnership()) ||
        (objectB == characterCollisionObject)) {
        if (aIsDynamic) {
            a->setShouldClaimSimulationOwnership(true);
        }
    }
}


void PhysicsEngine::computeCollisionEvents() {
    BT_PROFILE("computeCollisionEvents");

    // update all contacts every frame
    int numManifolds = _collisionDispatcher->getNumManifolds();
    for (int i = 0; i < numManifolds; ++i) {
        btPersistentManifold* contactManifold =  _collisionDispatcher->getManifoldByIndexInternal(i);
        if (contactManifold->getNumContacts() > 0) {
            // TODO: require scripts to register interest in callbacks for specific objects 
            // so we can filter out most collision events right here.
            const btCollisionObject* objectA = static_cast<const btCollisionObject*>(contactManifold->getBody0());
            const btCollisionObject* objectB = static_cast<const btCollisionObject*>(contactManifold->getBody1());

            if (!(objectA->isActive() || objectB->isActive())) {
                // both objects are inactive so stop tracking this contact, 
                // which will eventually trigger a CONTACT_EVENT_TYPE_END
                continue;
            }

            ObjectMotionState* a = static_cast<ObjectMotionState*>(objectA->getUserPointer());
            ObjectMotionState* b = static_cast<ObjectMotionState*>(objectB->getUserPointer());
            if (a || b) {
                // the manifold has up to 4 distinct points, but only extract info from the first
                _contactMap[ContactKey(a, b)].update(_numContactFrames, contactManifold->getContactPoint(0), _originOffset);
            }

            doOwnershipInfection(objectA, objectB);
        }
    }
    
    const uint32_t CONTINUE_EVENT_FILTER_FREQUENCY = 10;

    // scan known contacts and trigger events
    ContactMap::iterator contactItr = _contactMap.begin();

    while (contactItr != _contactMap.end()) {
        ObjectMotionState* A = static_cast<ObjectMotionState*>(contactItr->first._a);
        ObjectMotionState* B = static_cast<ObjectMotionState*>(contactItr->first._b);

        // TODO: make triggering these events clean and efficient.  The code at this context shouldn't 
        // have to figure out what kind of object (entity, avatar, etc) these are in order to properly 
        // emit a collision event.
        // TODO: enable scripts to filter based on contact event type
        ContactEventType type = contactItr->second.computeType(_numContactFrames);
        if(type != CONTACT_EVENT_TYPE_CONTINUE || _numSubsteps % CONTINUE_EVENT_FILTER_FREQUENCY == 0){
            if (A && A->getType() == MOTION_STATE_TYPE_ENTITY) {
                EntityItemID idA = static_cast<EntityMotionState*>(A)->getEntity()->getEntityItemID();
                EntityItemID idB;
                if (B && B->getType() == MOTION_STATE_TYPE_ENTITY) {
                    idB = static_cast<EntityMotionState*>(B)->getEntity()->getEntityItemID();
                }
                emit entityCollisionWithEntity(idA, idB, contactItr->second);
            } else if (B && B->getType() == MOTION_STATE_TYPE_ENTITY) {
                EntityItemID idA;
                EntityItemID idB = static_cast<EntityMotionState*>(B)->getEntity()->getEntityItemID();
                emit entityCollisionWithEntity(idA, idB, contactItr->second);
            }
        }

        if (type == CONTACT_EVENT_TYPE_END) {
            ContactMap::iterator iterToDelete = contactItr;
            ++contactItr;
            _contactMap.erase(iterToDelete);
        } else {
            ++contactItr;
        }
    }
    ++_numContactFrames;
}

void PhysicsEngine::dumpStatsIfNecessary() {
    if (_dumpNextStats) {
        _dumpNextStats = false;
        CProfileManager::dumpAll();
    }
}

// Bullet collision flags are as follows:
// CF_STATIC_OBJECT= 1,
// CF_KINEMATIC_OBJECT= 2,
// CF_NO_CONTACT_RESPONSE = 4,
// CF_CUSTOM_MATERIAL_CALLBACK = 8,//this allows per-triangle material (friction/restitution)
// CF_CHARACTER_OBJECT = 16,
// CF_DISABLE_VISUALIZE_OBJECT = 32, //disable debug drawing
// CF_DISABLE_SPU_COLLISION_PROCESSING = 64//disable parallel/SPU processing

void PhysicsEngine::addObject(const ShapeInfo& shapeInfo, btCollisionShape* shape, ObjectMotionState* motionState) {
    assert(shape);
    assert(motionState);

    btVector3 inertia(0.0f, 0.0f, 0.0f);
    float mass = 0.0f;
    btRigidBody* body = NULL;
    switch(motionState->computeObjectMotionType()) {
        case MOTION_TYPE_KINEMATIC: {
            body = new btRigidBody(mass, motionState, shape, inertia);
            body->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
            body->updateInertiaTensor();
            motionState->setRigidBody(body);
            motionState->setKinematic(true, _numSubsteps);
            const float KINEMATIC_LINEAR_VELOCITY_THRESHOLD = 0.01f;  // 1 cm/sec
            const float KINEMATIC_ANGULAR_VELOCITY_THRESHOLD = 0.01f;  // ~1 deg/sec
            body->setSleepingThresholds(KINEMATIC_LINEAR_VELOCITY_THRESHOLD, KINEMATIC_ANGULAR_VELOCITY_THRESHOLD);
            break;
        }
        case MOTION_TYPE_DYNAMIC: {
            mass = motionState->computeObjectMass(shapeInfo);
            shape->calculateLocalInertia(mass, inertia);
            body = new btRigidBody(mass, motionState, shape, inertia);
            body->updateInertiaTensor();
            motionState->setRigidBody(body);
            motionState->setKinematic(false, _numSubsteps);
            motionState->updateBodyVelocities();
            // NOTE: Bullet will deactivate any object whose velocity is below these thresholds for longer than 2 seconds.
            // (the 2 seconds is determined by: static btRigidBody::gDeactivationTime
            const float DYNAMIC_LINEAR_VELOCITY_THRESHOLD = 0.05f;  // 5 cm/sec
            const float DYNAMIC_ANGULAR_VELOCITY_THRESHOLD = 0.087266f;  // ~5 deg/sec
            body->setSleepingThresholds(DYNAMIC_LINEAR_VELOCITY_THRESHOLD, DYNAMIC_ANGULAR_VELOCITY_THRESHOLD);
            if (!motionState->isMoving()) {
                // try to initialize this object as inactive
                body->forceActivationState(ISLAND_SLEEPING);
            }
            break;
        }
        case MOTION_TYPE_STATIC:
        default: {
            body = new btRigidBody(mass, motionState, shape, inertia);
            body->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
            body->updateInertiaTensor();
            motionState->setRigidBody(body);
            motionState->setKinematic(false, _numSubsteps);
            break;
        }
    }
    body->setFlags(BT_DISABLE_WORLD_GRAVITY);
    motionState->updateBodyMaterialProperties();

    _dynamicsWorld->addRigidBody(body);
    motionState->resetMeasuredBodyAcceleration();
}

void PhysicsEngine::bump(EntityItem* bumpEntity) {
    // If this node is doing something like deleting an entity, scan for contacts involving the
    // entity.  For each found, flag the other entity involved as being simulated by this node.
    lock();
    int numManifolds = _collisionDispatcher->getNumManifolds();
    for (int i = 0; i < numManifolds; ++i) {
        btPersistentManifold* contactManifold =  _collisionDispatcher->getManifoldByIndexInternal(i);
        if (contactManifold->getNumContacts() > 0) {
            const btCollisionObject* objectA = static_cast<const btCollisionObject*>(contactManifold->getBody0());
            const btCollisionObject* objectB = static_cast<const btCollisionObject*>(contactManifold->getBody1());
            if (objectA && objectB) {
                void* a = objectA->getUserPointer();
                void* b = objectB->getUserPointer();
                if (a && b) {
                    EntityMotionState* entityMotionStateA = static_cast<EntityMotionState*>(a);
                    EntityMotionState* entityMotionStateB = static_cast<EntityMotionState*>(b);
                    EntityItem* entityA = entityMotionStateA ? entityMotionStateA->getEntity() : NULL;
                    EntityItem* entityB = entityMotionStateB ? entityMotionStateB->getEntity() : NULL;
                    if (entityA && entityB) {
                        if (entityA == bumpEntity) {
                            entityMotionStateB->setShouldClaimSimulationOwnership(true);
                            if (!objectB->isActive()) {
                                objectB->setActivationState(ACTIVE_TAG);
                            }
                        }
                        if (entityB == bumpEntity) {
                            entityMotionStateA->setShouldClaimSimulationOwnership(true);
                            if (!objectA->isActive()) {
                                objectA->setActivationState(ACTIVE_TAG);
                            }
                        }
                    }
                }
            }
        }
    }
    unlock();
}

void PhysicsEngine::removeObjectFromBullet(ObjectMotionState* motionState) {
    assert(motionState);
    btRigidBody* body = motionState->getRigidBody();

    // wake up anything touching this object
    EntityItem* entityItem = motionState ? motionState->getEntity() : NULL;
    bump(entityItem);

    if (body) {
        const btCollisionShape* shape = body->getCollisionShape();
        _dynamicsWorld->removeRigidBody(body);
        _shapeManager.releaseShape(shape);
        // NOTE: setRigidBody() modifies body->m_userPointer so we should clear the MotionState's body BEFORE deleting it.
        motionState->setRigidBody(NULL);
        delete body;
        motionState->setKinematic(false, _numSubsteps);

        removeContacts(motionState);
    }
}

// private
bool PhysicsEngine::updateBodyHard(btRigidBody* body, ObjectMotionState* motionState, uint32_t flags) {
    MotionType newType = motionState->computeObjectMotionType();

    // pull body out of physics engine
    _dynamicsWorld->removeRigidBody(body);

    if (flags & EntityItem::DIRTY_SHAPE) {
        // MASS bit should be set whenever SHAPE is set
        assert(flags & EntityItem::DIRTY_MASS);

        // get new shape
        btCollisionShape* oldShape = body->getCollisionShape();
        ShapeInfo shapeInfo;
        motionState->computeObjectShapeInfo(shapeInfo);
        btCollisionShape* newShape = _shapeManager.getShape(shapeInfo);
        if (!newShape) {
            // FAIL! we are unable to support these changes!
            _shapeManager.releaseShape(oldShape);

            // NOTE: setRigidBody() modifies body->m_userPointer so we should clear the MotionState's body BEFORE deleting it.
            motionState->setRigidBody(NULL);
            delete body;
            motionState->setKinematic(false, _numSubsteps);
            removeContacts(motionState);
            return false;
        } else if (newShape != oldShape) {
            // BUG: if shape doesn't change but density does then we won't compute new mass properties
            // TODO: fix this BUG by replacing DIRTY_MASS with DIRTY_DENSITY and then fix logic accordingly.
            body->setCollisionShape(newShape);
            _shapeManager.releaseShape(oldShape);

            // compute mass properties
            float mass = motionState->computeObjectMass(shapeInfo);
            btVector3 inertia(0.0f, 0.0f, 0.0f);
            body->getCollisionShape()->calculateLocalInertia(mass, inertia);
            body->setMassProps(mass, inertia);
            body->updateInertiaTensor();
        } else {
            // whoops, shape hasn't changed after all so we must release the reference
            // that was created when looking it up
            _shapeManager.releaseShape(newShape);
        }
    }
    bool easyUpdate = flags & EASY_DIRTY_PHYSICS_FLAGS;
    if (easyUpdate) {
        motionState->updateBodyEasy(flags, _numSubsteps);
    }

    // update the motion parameters
    switch (newType) {
        case MOTION_TYPE_KINEMATIC: {
            int collisionFlags = body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT;
            collisionFlags &= ~(btCollisionObject::CF_STATIC_OBJECT);
            body->setCollisionFlags(collisionFlags);
            body->forceActivationState(DISABLE_DEACTIVATION);

            body->setMassProps(0.0f, btVector3(0.0f, 0.0f, 0.0f));
            body->updateInertiaTensor();
            motionState->setKinematic(true, _numSubsteps);
            break;
        }
        case MOTION_TYPE_DYNAMIC: {
            int collisionFlags = body->getCollisionFlags() & ~(btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_STATIC_OBJECT);
            body->setCollisionFlags(collisionFlags);
            if (! (flags & EntityItem::DIRTY_MASS)) {
                // always update mass properties when going dynamic (unless it's already been done above)
                ShapeInfo shapeInfo;
                motionState->computeObjectShapeInfo(shapeInfo);
                float mass = motionState->computeObjectMass(shapeInfo);
                btVector3 inertia(0.0f, 0.0f, 0.0f);
                body->getCollisionShape()->calculateLocalInertia(mass, inertia);
                body->setMassProps(mass, inertia);
                body->updateInertiaTensor();
            }
            body->forceActivationState(ACTIVE_TAG);
            motionState->setKinematic(false, _numSubsteps);
            break;
        }
        default: {
            // MOTION_TYPE_STATIC
            int collisionFlags = body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT;
            collisionFlags &= ~(btCollisionObject::CF_KINEMATIC_OBJECT);
            body->setCollisionFlags(collisionFlags);
            body->forceActivationState(DISABLE_SIMULATION);

            body->setMassProps(0.0f, btVector3(0.0f, 0.0f, 0.0f));
            body->updateInertiaTensor();

            body->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
            body->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
            motionState->setKinematic(false, _numSubsteps);
            break;
        }
    }

    // reinsert body into physics engine
    _dynamicsWorld->addRigidBody(body);

    body->activate();
    return true;
}

void PhysicsEngine::setCharacterController(DynamicCharacterController* character) {
    if (_characterController != character) {
        lock();
        if (_characterController) {
            // remove the character from the DynamicsWorld immediately
            _characterController->setDynamicsWorld(NULL);
            _characterController = NULL;
        }
        // the character will be added to the DynamicsWorld later
        _characterController = character;
        unlock();
    }
}

