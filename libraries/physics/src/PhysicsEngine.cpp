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

#include "ObjectMotionState.h"
#include "PhysicsEngine.h"
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
    _characterController(nullptr) {
}

PhysicsEngine::~PhysicsEngine() {
    if (_characterController) {
        _characterController->setDynamicsWorld(nullptr);
    }
    // TODO: delete engine components... if we ever plan to create more than one instance
    delete _collisionConfig;
    delete _collisionDispatcher;
    delete _broadphaseFilter;
    delete _constraintSolver;
    delete _dynamicsWorld;
    delete _ghostPairCallback;
}

void PhysicsEngine::init() {
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
}

void PhysicsEngine::removeObjects(VectorOfMotionStates& objects) {
    for (auto object : objects) {
        assert(object);
    
        // wake up anything touching this object
        bump(object);

        btRigidBody* body = object->getRigidBody();
        _dynamicsWorld->removeRigidBody(body);

        // NOTE: setRigidBody() modifies body->m_userPointer so we should clear the MotionState's body BEFORE deleting it.
        object->setRigidBody(nullptr);
        delete body;
        removeContacts(object);
        object->releaseShape();
    }
}

void PhysicsEngine::addObjects(VectorOfMotionStates& objects) {
    for (auto object : objects) {
        assert(object);
        addObject(object);
    }
}

void PhysicsEngine::changeObjects(VectorOfMotionStates& objects) {
    for (auto object : objects) {
        uint32_t flags = object->getIncomingDirtyFlags() & DIRTY_PHYSICS_FLAGS;
        if (flags & HARD_DIRTY_PHYSICS_FLAGS) {
            object->handleHardAndEasyChanges(flags, this);
        } else {
            object->handleEasyChanges(flags);
        }
    }
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

void PhysicsEngine::stepSimulation() {
    CProfileManager::Reset();
    BT_PROFILE("stepSimulation");
    // NOTE: the grand order of operations is:
    // (1) pull incoming changes
    // (2) step simulation
    // (3) synchronize outgoing motion states
    // (4) send outgoing packets

    const int MAX_NUM_SUBSTEPS = 4;
    const float MAX_TIMESTEP = (float)MAX_NUM_SUBSTEPS * PHYSICS_ENGINE_FIXED_SUBSTEP;
    float dt = 1.0e-6f * (float)(_clock.getTimeMicroseconds());
    _clock.reset();
    float timeStep = btMin(dt, MAX_TIMESTEP);

    // TODO: move character->preSimulation() into relayIncomingChanges
    if (_characterController) {
        if (_characterController->needsRemoval()) {
            _characterController->setDynamicsWorld(nullptr);
        }
        _characterController->updateShapeIfNecessary();
        if (_characterController->needsAddition()) {
            _characterController->setDynamicsWorld(_dynamicsWorld);
        }
        _characterController->preSimulation(timeStep);
    }

    int numSubsteps = _dynamicsWorld->stepSimulation(timeStep, MAX_NUM_SUBSTEPS, PHYSICS_ENGINE_FIXED_SUBSTEP);
    if (numSubsteps > 0) {
        BT_PROFILE("postSimulation");
        _numSubsteps += (uint32_t)numSubsteps;
        ObjectMotionState::setWorldSimulationStep(_numSubsteps);

        if (_characterController) {
            _characterController->postSimulation();
        }
        computeCollisionEvents();
        _hasOutgoingChanges = true;
    }
}

void PhysicsEngine::doOwnershipInfection(const btCollisionObject* objectA, const btCollisionObject* objectB) {
    BT_PROFILE("ownershipInfection");
    assert(objectA);
    assert(objectB);

    auto nodeList = DependencyManager::get<NodeList>();
    QUuid myNodeID = nodeList->getSessionUUID();
    const btCollisionObject* characterCollisionObject =
        _characterController ? _characterController->getCollisionObject() : nullptr;

    assert(!myNodeID.isNull());

    ObjectMotionState* a = static_cast<ObjectMotionState*>(objectA->getUserPointer());
    ObjectMotionState* b = static_cast<ObjectMotionState*>(objectB->getUserPointer());
    EntityItem* entityA = a ? a->getEntity() : nullptr;
    EntityItem* entityB = b ? b->getEntity() : nullptr;
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

VectorOfMotionStates& PhysicsEngine::getOutgoingChanges() {
    BT_PROFILE("copyOutgoingChanges");
    _dynamicsWorld.synchronizeMotionStates();
    _hasOutgoingChanges = false;
    return _dynamicsWorld.getChangedMotionStates();
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

void PhysicsEngine::addObject(ObjectMotionState* motionState) {
    assert(motionState);
    btCollisionShape* shape = motionState->getShape();
    assert(shape);

    btVector3 inertia(0.0f, 0.0f, 0.0f);
    float mass = 0.0f;
    btRigidBody* body = nullptr;
    switch(motionState->computeObjectMotionType()) {
        case MOTION_TYPE_KINEMATIC: {
            body = new btRigidBody(mass, motionState, shape, inertia);
            body->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
            body->updateInertiaTensor();
            motionState->setRigidBody(body);
            motionState->updateBodyVelocities();
            const float KINEMATIC_LINEAR_VELOCITY_THRESHOLD = 0.01f;  // 1 cm/sec
            const float KINEMATIC_ANGULAR_VELOCITY_THRESHOLD = 0.01f;  // ~1 deg/sec
            body->setSleepingThresholds(KINEMATIC_LINEAR_VELOCITY_THRESHOLD, KINEMATIC_ANGULAR_VELOCITY_THRESHOLD);
            break;
        }
        case MOTION_TYPE_DYNAMIC: {
            mass = motionState->getMass();
            shape->calculateLocalInertia(mass, inertia);
            body = new btRigidBody(mass, motionState, shape, inertia);
            body->updateInertiaTensor();
            motionState->setRigidBody(body);
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
            break;
        }
    }
    body->setFlags(BT_DISABLE_WORLD_GRAVITY);
    motionState->updateBodyMaterialProperties();

    _dynamicsWorld->addRigidBody(body);
    motionState->resetMeasuredBodyAcceleration();
}

/* TODO: convert bump() to take an ObjectMotionState.
* Expose SimulationID to ObjectMotionState*/
void PhysicsEngine::bump(EntityItem* bumpEntity) {
    // If this node is doing something like deleting an entity, scan for contacts involving the
    // entity.  For each found, flag the other entity involved as being simulated by this node.
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
                    EntityItem* entityA = entityMotionStateA ? entityMotionStateA->getEntity() : nullptr;
                    EntityItem* entityB = entityMotionStateB ? entityMotionStateB->getEntity() : nullptr;
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
}

void PhysicsEngine::removeRigidBody(btRigidBody* body) {
    // pull body out of physics engine
    _dynamicsWorld->removeRigidBody(body);
}

void PhysicsEngine::addRigidBody(btRigidBody* body, MotionType motionType, float mass) {
    switch (motionType) {
        case MOTION_TYPE_KINEMATIC: {
            int collisionFlags = body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT;
            collisionFlags &= ~(btCollisionObject::CF_STATIC_OBJECT);
            body->setCollisionFlags(collisionFlags);
            body->forceActivationState(DISABLE_DEACTIVATION);

            body->setMassProps(0.0f, btVector3(0.0f, 0.0f, 0.0f));
            body->updateInertiaTensor();
            break;
        }
        case MOTION_TYPE_DYNAMIC: {
            int collisionFlags = body->getCollisionFlags() & ~(btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_STATIC_OBJECT);
            body->setCollisionFlags(collisionFlags);
            if (! (flags & EntityItem::DIRTY_MASS)) {
                // always update mass properties when going dynamic (unless it's already been done above)
                btVector3 inertia(0.0f, 0.0f, 0.0f);
                body->getCollisionShape()->calculateLocalInertia(mass, inertia);
                body->setMassProps(mass, inertia);
                body->updateInertiaTensor();
            }
            body->forceActivationState(ACTIVE_TAG);
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
            break;
        }
    }

    _dynamicsWorld->addRigidBody(body);

    body->activate();
}

void PhysicsEngine::setCharacterController(DynamicCharacterController* character) {
    if (_characterController != character) {
        if (_characterController) {
            // remove the character from the DynamicsWorld immediately
            _characterController->setDynamicsWorld(nullptr);
            _characterController = nullptr;
        }
        // the character will be added to the DynamicsWorld later
        _characterController = character;
    }
}

