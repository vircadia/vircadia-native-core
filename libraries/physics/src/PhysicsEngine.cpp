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

#include <PhysicsCollisionGroups.h>

#include "ObjectMotionState.h"
#include "PhysicsEngine.h"
#include "PhysicsHelpers.h"
#include "ThreadSafeDynamicsWorld.h"
#include "PhysicsLogging.h"

uint32_t PhysicsEngine::getNumSubsteps() {
    return _numSubsteps;
}

PhysicsEngine::PhysicsEngine(const glm::vec3& offset) :
        _originOffset(offset),
        _characterController(nullptr) {
    // build table of masks with their group as the key
    _collisionMasks.insert(btHashInt((int)COLLISION_GROUP_DEFAULT), COLLISION_MASK_DEFAULT);
    _collisionMasks.insert(btHashInt((int)COLLISION_GROUP_STATIC), COLLISION_MASK_STATIC);
    _collisionMasks.insert(btHashInt((int)COLLISION_GROUP_KINEMATIC), COLLISION_MASK_KINEMATIC);
    _collisionMasks.insert(btHashInt((int)COLLISION_GROUP_DEBRIS), COLLISION_MASK_DEBRIS);
    _collisionMasks.insert(btHashInt((int)COLLISION_GROUP_TRIGGER), COLLISION_MASK_TRIGGER);
    _collisionMasks.insert(btHashInt((int)COLLISION_GROUP_MY_AVATAR), COLLISION_MASK_MY_AVATAR);
    _collisionMasks.insert(btHashInt((int)COLLISION_GROUP_MY_ATTACHMENT), COLLISION_MASK_MY_ATTACHMENT);
    _collisionMasks.insert(btHashInt((int)COLLISION_GROUP_OTHER_AVATAR), COLLISION_MASK_OTHER_AVATAR);
    _collisionMasks.insert(btHashInt((int)COLLISION_GROUP_OTHER_ATTACHMENT), COLLISION_MASK_OTHER_ATTACHMENT);
    _collisionMasks.insert(btHashInt((int)COLLISION_GROUP_COLLISIONLESS), COLLISION_MASK_COLLISIONLESS);
}

PhysicsEngine::~PhysicsEngine() {
    if (_characterController) {
        _characterController->setDynamicsWorld(nullptr);
    }
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

void PhysicsEngine::addObject(ObjectMotionState* motionState) {
    assert(motionState);

    btVector3 inertia(0.0f, 0.0f, 0.0f);
    float mass = 0.0f;
    // NOTE: the body may or may not already exist, depending on whether this corresponds to a reinsertion, or a new insertion.
    btRigidBody* body = motionState->getRigidBody();
    MotionType motionType = motionState->computeObjectMotionType();
    motionState->setMotionType(motionType);
    switch(motionType) {
        case MOTION_TYPE_KINEMATIC: {
            if (!body) {
                btCollisionShape* shape = motionState->getShape();
                assert(shape);
                body = new btRigidBody(mass, motionState, shape, inertia);
            } else {
                body->setMassProps(mass, inertia);
            }
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
            btCollisionShape* shape = motionState->getShape();
            assert(shape);
            shape->calculateLocalInertia(mass, inertia);
            if (!body) {
                body = new btRigidBody(mass, motionState, shape, inertia);
            } else {
                body->setMassProps(mass, inertia);
            }
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
            if (!body) {
                assert(motionState->getShape());
                body = new btRigidBody(mass, motionState, motionState->getShape(), inertia);
            } else {
                body->setMassProps(mass, inertia);
            }
            body->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
            body->updateInertiaTensor();
            motionState->setRigidBody(body);
            break;
        }
    }
    body->setFlags(BT_DISABLE_WORLD_GRAVITY);
    motionState->updateBodyMaterialProperties();

    int16_t group = motionState->computeCollisionGroup();
    _dynamicsWorld->addRigidBody(body, group, getCollisionMask(group));

    motionState->clearIncomingDirtyFlags();
}

void PhysicsEngine::removeObject(ObjectMotionState* object) {
    // wake up anything touching this object
    bump(object);
    removeContacts(object);

    btRigidBody* body = object->getRigidBody();
    assert(body);
    _dynamicsWorld->removeRigidBody(body);
}

void PhysicsEngine::deleteObjects(const VectorOfMotionStates& objects) {
    for (auto object : objects) {
        removeObject(object);

        // NOTE: setRigidBody() modifies body->m_userPointer so we should clear the MotionState's body BEFORE deleting it.
        btRigidBody* body = object->getRigidBody();
        object->setRigidBody(nullptr);
        body->setMotionState(nullptr);
        delete body;
        object->releaseShape();
        delete object;
    }
}

// Same as above, but takes a Set instead of a Vector.  Should only be called during teardown.
void PhysicsEngine::deleteObjects(const SetOfMotionStates& objects) {
    for (auto object : objects) {
        btRigidBody* body = object->getRigidBody();
        removeObject(object);

        // NOTE: setRigidBody() modifies body->m_userPointer so we should clear the MotionState's body BEFORE deleting it.
        object->setRigidBody(nullptr);
        body->setMotionState(nullptr);
        delete body;
        object->releaseShape();
        delete object;
    }
}

void PhysicsEngine::addObjects(const VectorOfMotionStates& objects) {
    for (auto object : objects) {
        addObject(object);
    }
}

VectorOfMotionStates PhysicsEngine::changeObjects(const VectorOfMotionStates& objects) {
    VectorOfMotionStates stillNeedChange;
    for (auto object : objects) {
        uint32_t flags = object->getIncomingDirtyFlags() & DIRTY_PHYSICS_FLAGS;
        if (flags & HARD_DIRTY_PHYSICS_FLAGS) {
            if (object->handleHardAndEasyChanges(flags, this)) {
                object->clearIncomingDirtyFlags();
            } else {
                stillNeedChange.push_back(object);
            }
        } else if (flags & EASY_DIRTY_PHYSICS_FLAGS) {
            if (object->handleEasyChanges(flags, this)) {
                object->clearIncomingDirtyFlags();
            } else {
                stillNeedChange.push_back(object);
            }
        }
    }
    return stillNeedChange;
}

void PhysicsEngine::reinsertObject(ObjectMotionState* object) {
    removeObject(object);
    addObject(object);
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

    const float MAX_TIMESTEP = (float)PHYSICS_ENGINE_MAX_NUM_SUBSTEPS * PHYSICS_ENGINE_FIXED_SUBSTEP;
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

    int numSubsteps = _dynamicsWorld->stepSimulation(timeStep, PHYSICS_ENGINE_MAX_NUM_SUBSTEPS, PHYSICS_ENGINE_FIXED_SUBSTEP);
    if (numSubsteps > 0) {
        BT_PROFILE("postSimulation");
        _numSubsteps += (uint32_t)numSubsteps;
        ObjectMotionState::setWorldSimulationStep(_numSubsteps);

        if (_characterController) {
            _characterController->postSimulation();
        }
        updateContactMap();
        _hasOutgoingChanges = true;
    }
}

void PhysicsEngine::doOwnershipInfection(const btCollisionObject* objectA, const btCollisionObject* objectB) {
    BT_PROFILE("ownershipInfection");

    const btCollisionObject* characterObject = _characterController ? _characterController->getCollisionObject() : nullptr;

    ObjectMotionState* motionStateA = static_cast<ObjectMotionState*>(objectA->getUserPointer());
    ObjectMotionState* motionStateB = static_cast<ObjectMotionState*>(objectB->getUserPointer());

    if (motionStateB &&
        ((motionStateA && motionStateA->getSimulatorID() == _sessionID && !objectA->isStaticObject()) ||
         (objectA == characterObject))) {
        // NOTE: we might own the simulation of a kinematic object (A)
        // but we don't claim ownership of kinematic objects (B) based on collisions here.
        if (!objectB->isStaticOrKinematicObject() && motionStateB->getSimulatorID() != _sessionID) {
            quint8 priority = motionStateA ? motionStateA->getSimulationPriority() : PERSONAL_SIMULATION_PRIORITY;
            motionStateB->bump(priority);
        }
    } else if (motionStateA &&
               ((motionStateB && motionStateB->getSimulatorID() == _sessionID && !objectB->isStaticObject()) ||
                (objectB == characterObject))) {
        // SIMILARLY: we might own the simulation of a kinematic object (B)
        // but we don't claim ownership of kinematic objects (A) based on collisions here.
        if (!objectA->isStaticOrKinematicObject() && motionStateA->getSimulatorID() != _sessionID) {
            quint8 priority = motionStateB ? motionStateB->getSimulationPriority() : PERSONAL_SIMULATION_PRIORITY;
            motionStateA->bump(priority);
        }
    }
}

void PhysicsEngine::updateContactMap() {
    BT_PROFILE("updateContactMap");
    ++_numContactFrames;

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
                _contactMap[ContactKey(a, b)].update(_numContactFrames, contactManifold->getContactPoint(0));
            }

            if (!_sessionID.isNull()) {
                doOwnershipInfection(objectA, objectB);
            }
        }
    }
}

const CollisionEvents& PhysicsEngine::getCollisionEvents() {
    const uint32_t CONTINUE_EVENT_FILTER_FREQUENCY = 10;
    _collisionEvents.clear();

    // scan known contacts and trigger events
    ContactMap::iterator contactItr = _contactMap.begin();

    while (contactItr != _contactMap.end()) {
        ContactInfo& contact = contactItr->second;
        ContactEventType type = contact.computeType(_numContactFrames);
        if(type != CONTACT_EVENT_TYPE_CONTINUE || _numSubsteps % CONTINUE_EVENT_FILTER_FREQUENCY == 0) {
            ObjectMotionState* motionStateA = static_cast<ObjectMotionState*>(contactItr->first._a);
            ObjectMotionState* motionStateB = static_cast<ObjectMotionState*>(contactItr->first._b);
            glm::vec3 velocityChange = (motionStateA ? motionStateA->getObjectLinearVelocityChange() : glm::vec3(0.0f)) +
                (motionStateB ? motionStateB->getObjectLinearVelocityChange() : glm::vec3(0.0f));

            if (motionStateA && motionStateA->getType() == MOTIONSTATE_TYPE_ENTITY) {
                QUuid idA = motionStateA->getObjectID();
                QUuid idB;
                if (motionStateB && motionStateB->getType() == MOTIONSTATE_TYPE_ENTITY) {
                    idB = motionStateB->getObjectID();
                }
                glm::vec3 position = bulletToGLM(contact.getPositionWorldOnB()) + _originOffset;
                glm::vec3 penetration = bulletToGLM(contact.distance * contact.normalWorldOnB);
                _collisionEvents.push_back(Collision(type, idA, idB, position, penetration, velocityChange));
            } else if (motionStateB && motionStateB->getType() == MOTIONSTATE_TYPE_ENTITY) {
                QUuid idB = motionStateB->getObjectID();
                glm::vec3 position = bulletToGLM(contact.getPositionWorldOnA()) + _originOffset;
                // NOTE: we're flipping the order of A and B (so that the first objectID is never NULL)
                // hence we must negate the penetration.
                glm::vec3 penetration = - bulletToGLM(contact.distance * contact.normalWorldOnB);
                _collisionEvents.push_back(Collision(type, idB, QUuid(), position, penetration, velocityChange));
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
    return _collisionEvents;
}

const VectorOfMotionStates& PhysicsEngine::getOutgoingChanges() {
    BT_PROFILE("copyOutgoingChanges");
    _dynamicsWorld->synchronizeMotionStates();
    _hasOutgoingChanges = false;
    return _dynamicsWorld->getChangedMotionStates();
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

void PhysicsEngine::bump(ObjectMotionState* motionState) {
    // Find all objects that touch the object corresponding to motionState and flag the other objects
    // for simulation ownership by the local simulation.

    assert(motionState);
    btCollisionObject* object = motionState->getRigidBody();

    int numManifolds = _collisionDispatcher->getNumManifolds();
    for (int i = 0; i < numManifolds; ++i) {
        btPersistentManifold* contactManifold =  _collisionDispatcher->getManifoldByIndexInternal(i);
        if (contactManifold->getNumContacts() > 0) {
            const btCollisionObject* objectA = static_cast<const btCollisionObject*>(contactManifold->getBody0());
            const btCollisionObject* objectB = static_cast<const btCollisionObject*>(contactManifold->getBody1());
            if (objectB == object) {
                if (!objectA->isStaticOrKinematicObject()) {
                    ObjectMotionState* motionStateA = static_cast<ObjectMotionState*>(objectA->getUserPointer());
                    if (motionStateA) {
                        motionStateA->bump(VOLUNTEER_SIMULATION_PRIORITY);
                        objectA->setActivationState(ACTIVE_TAG);
                    }
                }
            } else if (objectA == object) {
                if (!objectB->isStaticOrKinematicObject()) {
                    ObjectMotionState* motionStateB = static_cast<ObjectMotionState*>(objectB->getUserPointer());
                    if (motionStateB) {
                        motionStateB->bump(VOLUNTEER_SIMULATION_PRIORITY);
                        objectB->setActivationState(ACTIVE_TAG);
                    }
                }
            }
        }
    }
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

int16_t PhysicsEngine::getCollisionMask(int16_t group) const {
    const int16_t* mask = _collisionMasks.find(btHashInt((int)group));
    return mask ? *mask : COLLISION_MASK_DEFAULT;
}

EntityActionPointer PhysicsEngine::getActionByID(const QUuid& actionID) const {
    if (_objectActions.contains(actionID)) {
        return _objectActions[actionID];
    }
    return nullptr;
}

void PhysicsEngine::addAction(EntityActionPointer action) {
    assert(action);
    const QUuid& actionID = action->getID();
    if (_objectActions.contains(actionID)) {
        if (_objectActions[actionID] == action) {
            return;
        }
        removeAction(action->getID());
    }

    _objectActions[actionID] = action;

    // bullet needs a pointer to the action, but it doesn't use shared pointers.
    // is there a way to bump the reference count?
    ObjectAction* objectAction = static_cast<ObjectAction*>(action.get());
    _dynamicsWorld->addAction(objectAction);
}

void PhysicsEngine::removeAction(const QUuid actionID) {
    if (_objectActions.contains(actionID)) {
        EntityActionPointer action = _objectActions[actionID];
        ObjectAction* objectAction = static_cast<ObjectAction*>(action.get());
        _dynamicsWorld->removeAction(objectAction);
        _objectActions.remove(actionID);
    }
}
