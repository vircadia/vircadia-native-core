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

#include "PhysicsEngine.h"
#include "ShapeInfoUtil.h"
#include "PhysicsHelpers.h"
#include "ThreadSafeDynamicsWorld.h"

static uint32_t _numSubsteps;

// static
uint32_t PhysicsEngine::getNumSubsteps() {
    return _numSubsteps;
}

PhysicsEngine::PhysicsEngine(const glm::vec3& offset)
    :  _originOffset(offset) {
}

PhysicsEngine::~PhysicsEngine() {
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
            //qDebug() << "failed to add entity " << entity->getEntityItemID() << " to physics engine";
        }
    }
}

void PhysicsEngine::removeEntityInternal(EntityItem* entity) {
    assert(entity);
    void* physicsInfo = entity->getPhysicsInfo();
    if (physicsInfo) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(physicsInfo);
        if (motionState->getRigidBody()) {
            removeObject(motionState);
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
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
        _incomingChanges.insert(motionState);
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
        removeObject(*stateItr);
        delete (*stateItr);
    }
    _entityMotionStates.clear();
    _nonPhysicalKinematicObjects.clear();
    _incomingChanges.clear();
    _outgoingPackets.clear();
}
// end EntitySimulation overrides

void PhysicsEngine::relayIncomingChangesToSimulation() {
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
                bool success = updateObjectHard(body, motionState, flags);
                if (!success) {
                    // NOTE: since updateObjectHard() failed we know that motionState has been removed 
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
                motionState->updateObjectEasy(flags, _numSubsteps);
            }
        } else {
            // the only way we should ever get here (motionState exists but no body) is when the object
            // is undergoing non-physical kinematic motion.
            assert(_nonPhysicalKinematicObjects.contains(motionState));

            // it is possible that the changes are such that the object can now be added to the physical simulation
            if (flags & EntityItem::DIRTY_SHAPE) {
                ShapeInfo shapeInfo;
                motionState->computeShapeInfo(shapeInfo);
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
    // NOTE: the grand order of operations is:
    // (1) relay incoming changes
    // (2) step simulation
    // (3) synchronize outgoing motion states
    // (4) send outgoing packets

    // This is step (1).
    relayIncomingChangesToSimulation();

    const int MAX_NUM_SUBSTEPS = 4;
    const float MAX_TIMESTEP = (float)MAX_NUM_SUBSTEPS * PHYSICS_ENGINE_FIXED_SUBSTEP;
    float dt = 1.0e-6f * (float)(_clock.getTimeMicroseconds());
    _clock.reset();
    float timeStep = btMin(dt, MAX_TIMESTEP);

    // This is step (2).
    int numSubsteps = _dynamicsWorld->stepSimulation(timeStep, MAX_NUM_SUBSTEPS, PHYSICS_ENGINE_FIXED_SUBSTEP);
    _numSubsteps += (uint32_t)numSubsteps;
    stepNonPhysicalKinematics(usecTimestampNow());
    unlock();

    if (numSubsteps > 0) {
        // This is step (3) which is done outside of stepSimulation() so we can lock _entityTree.
        //
        // Unfortunately we have to unlock the simulation (above) before we try to lock the _entityTree
        // to avoid deadlock -- the _entityTree may try to lock its EntitySimulation (from which this 
        // PhysicsEngine derives) when updating/adding/deleting entities so we need to wait for our own
        // lock on the tree before we re-lock ourselves.
        //
        // TODO: untangle these lock sequences.
        _entityTree->lockForWrite();
        lock();
        _dynamicsWorld->synchronizeMotionStates();
        unlock();
        _entityTree->unlock();
    
        computeCollisionEvents();
    }
}

void PhysicsEngine::stepNonPhysicalKinematics(const quint64& now) {
    QSet<ObjectMotionState*>::iterator stateItr = _nonPhysicalKinematicObjects.begin();
    while (stateItr != _nonPhysicalKinematicObjects.end()) {
        ObjectMotionState* motionState = *stateItr;
        motionState->stepKinematicSimulation(now);
        ++stateItr;
    }
}

// TODO?: need to occasionally scan for stopped non-physical kinematics objects

void PhysicsEngine::computeCollisionEvents() {
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
        
            void* a = objectA->getUserPointer();
            void* b = objectB->getUserPointer();
            if (a || b) {
                // the manifold has up to 4 distinct points, but only extract info from the first
                _contactMap[ContactKey(a, b)].update(_numContactFrames, contactManifold->getContactPoint(0), _originOffset);
            }
        }
    }
    
    // We harvest collision callbacks every few frames, which contributes the following effects:
    //
    // (1) There is a maximum collision callback rate per pair:  substep_rate / SUBSTEPS_PER_COLLIION_FRAME
    // (2) END/START cycles shorter than SUBSTEPS_PER_COLLIION_FRAME will be filtered out
    // (3) There is variable lag between when the contact actually starts and when it is reported, 
    //     up to SUBSTEPS_PER_COLLIION_FRAME * time_per_substep
    //
    const uint32_t SUBSTEPS_PER_COLLISION_FRAME = 2;
    if (_numSubsteps - _numContactFrames * SUBSTEPS_PER_COLLISION_FRAME < SUBSTEPS_PER_COLLISION_FRAME) {
        // we don't harvest collision callbacks every frame
        // this sets a maximum callback-per-contact rate
        // and also filters out END/START events that happen on shorter timescales
        return;
    }

    ++_numContactFrames;
    // scan known contacts and trigger events
    ContactMap::iterator contactItr = _contactMap.begin();
    while (contactItr != _contactMap.end()) {
        ObjectMotionState* A = static_cast<ObjectMotionState*>(contactItr->first._a);
        ObjectMotionState* B = static_cast<ObjectMotionState*>(contactItr->first._b);

        // TODO: make triggering these events clean and efficient.  The code at this context shouldn't 
        // have to figure out what kind of object (entity, avatar, etc) these are in order to properly 
        // emit a collision event.
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

        // TODO: enable scripts to filter based on contact event type
        ContactEventType type = contactItr->second.computeType(_numContactFrames);
        if (type == CONTACT_EVENT_TYPE_END) {
            ContactMap::iterator iterToDelete = contactItr;
            ++contactItr;
            _contactMap.erase(iterToDelete);
        } else {
            ++contactItr;
        }
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
    switch(motionState->computeMotionType()) {
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
            mass = motionState->computeMass(shapeInfo);
            shape->calculateLocalInertia(mass, inertia);
            body = new btRigidBody(mass, motionState, shape, inertia);
            body->updateInertiaTensor();
            motionState->setRigidBody(body);
            motionState->setKinematic(false, _numSubsteps);
            motionState->updateObjectVelocities();
            // NOTE: Bullet will deactivate any object whose velocity is below these thresholds for longer than 2 seconds.
            // (the 2 seconds is determined by: static btRigidBody::gDeactivationTime
            const float DYNAMIC_LINEAR_VELOCITY_THRESHOLD = 0.05f;  // 5 cm/sec
            const float DYNAMIC_ANGULAR_VELOCITY_THRESHOLD = 0.087266f;  // ~5 deg/sec
            body->setSleepingThresholds(DYNAMIC_LINEAR_VELOCITY_THRESHOLD, DYNAMIC_ANGULAR_VELOCITY_THRESHOLD);
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
    body->setRestitution(motionState->_restitution);
    body->setFriction(motionState->_friction);
    body->setDamping(motionState->_linearDamping, motionState->_angularDamping);
    _dynamicsWorld->addRigidBody(body);
}

void PhysicsEngine::removeObject(ObjectMotionState* motionState) {
    assert(motionState);
    btRigidBody* body = motionState->getRigidBody();
    if (body) {
        const btCollisionShape* shape = body->getCollisionShape();
        ShapeInfo shapeInfo;
        ShapeInfoUtil::collectInfoFromShape(shape, shapeInfo);
        _dynamicsWorld->removeRigidBody(body);
        _shapeManager.releaseShape(shapeInfo);
        delete body;
        motionState->setRigidBody(NULL);
        motionState->setKinematic(false, _numSubsteps);

        removeContacts(motionState);
    }
}

// private
bool PhysicsEngine::updateObjectHard(btRigidBody* body, ObjectMotionState* motionState, uint32_t flags) {
    MotionType newType = motionState->computeMotionType();

    // pull body out of physics engine
    _dynamicsWorld->removeRigidBody(body);

    if (flags & EntityItem::DIRTY_SHAPE) {
        // MASS bit should be set whenever SHAPE is set
        assert(flags & EntityItem::DIRTY_MASS);

        // get new shape
        btCollisionShape* oldShape = body->getCollisionShape();
        ShapeInfo shapeInfo;
        motionState->computeShapeInfo(shapeInfo);
        btCollisionShape* newShape = _shapeManager.getShape(shapeInfo);
        if (!newShape) {
            // FAIL! we are unable to support these changes!
            _shapeManager.releaseShape(oldShape);

            delete body;
            motionState->setRigidBody(NULL);
            motionState->setKinematic(false, _numSubsteps);
            removeContacts(motionState);
            return false;
        } else if (newShape != oldShape) {
            // BUG: if shape doesn't change but density does then we won't compute new mass properties
            // TODO: fix this BUG by replacing DIRTY_MASS with DIRTY_DENSITY and then fix logic accordingly.
            body->setCollisionShape(newShape);
            _shapeManager.releaseShape(oldShape);

            // compute mass properties
            float mass = motionState->computeMass(shapeInfo);
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
        motionState->updateObjectEasy(flags, _numSubsteps);
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
                motionState->computeShapeInfo(shapeInfo);
                float mass = motionState->computeMass(shapeInfo);
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
