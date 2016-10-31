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

#include <PerfStat.h>

#include "CharacterController.h"
#include "ObjectMotionState.h"
#include "PhysicsEngine.h"
#include "PhysicsHelpers.h"
#include "ThreadSafeDynamicsWorld.h"
#include "PhysicsLogging.h"

PhysicsEngine::PhysicsEngine(const glm::vec3& offset) :
        _originOffset(offset),
        _myAvatarController(nullptr) {
}

PhysicsEngine::~PhysicsEngine() {
    if (_myAvatarController) {
        _myAvatarController->setDynamicsWorld(nullptr);
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

        // By default Bullet will update the Aabb's of all objects every frame, even statics.
        // This can waste CPU cycles so we configure Bullet to only update ACTIVE objects here.
        // However, this means when a static object is moved we must manually update its Aabb
        // in order for its broadphase collision queries to work correctly. Look at how we use
        // _activeStaticBodies to track and update the Aabb's of moved static objects.
        _dynamicsWorld->setForceUpdateAllAabbs(false);
    }
}

uint32_t PhysicsEngine::getNumSubsteps() {
    return _numSubsteps;
}

// private
void PhysicsEngine::addObjectToDynamicsWorld(ObjectMotionState* motionState) {
    assert(motionState);

    btVector3 inertia(0.0f, 0.0f, 0.0f);
    float mass = 0.0f;
    // NOTE: the body may or may not already exist, depending on whether this corresponds to a reinsertion, or a new insertion.
    btRigidBody* body = motionState->getRigidBody();
    PhysicsMotionType motionType = motionState->computePhysicsMotionType();
    motionState->setMotionType(motionType);
    switch(motionType) {
        case MOTION_TYPE_KINEMATIC: {
            if (!body) {
                btCollisionShape* shape = const_cast<btCollisionShape*>(motionState->getShape());
                assert(shape);
                body = new btRigidBody(mass, motionState, shape, inertia);
                motionState->setRigidBody(body);
            } else {
                body->setMassProps(mass, inertia);
            }
            body->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
            body->updateInertiaTensor();
            motionState->updateBodyVelocities();
            motionState->updateLastKinematicStep();
            body->setSleepingThresholds(KINEMATIC_LINEAR_SPEED_THRESHOLD, KINEMATIC_ANGULAR_SPEED_THRESHOLD);
            motionState->clearInternalKinematicChanges();
            break;
        }
        case MOTION_TYPE_DYNAMIC: {
            mass = motionState->getMass();
            btCollisionShape* shape = const_cast<btCollisionShape*>(motionState->getShape());
            assert(shape);
            shape->calculateLocalInertia(mass, inertia);
            if (!body) {
                body = new btRigidBody(mass, motionState, shape, inertia);
                motionState->setRigidBody(body);
            } else {
                body->setMassProps(mass, inertia);
            }
            body->setCollisionFlags(body->getCollisionFlags() & ~(btCollisionObject::CF_KINEMATIC_OBJECT |
                                                                  btCollisionObject::CF_STATIC_OBJECT));
            body->updateInertiaTensor();
            motionState->updateBodyVelocities();

            // NOTE: Bullet will deactivate any object whose velocity is below these thresholds for longer than 2 seconds.
            // (the 2 seconds is determined by: static btRigidBody::gDeactivationTime
            body->setSleepingThresholds(DYNAMIC_LINEAR_SPEED_THRESHOLD, DYNAMIC_ANGULAR_SPEED_THRESHOLD);
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
                body = new btRigidBody(mass, motionState, const_cast<btCollisionShape*>(motionState->getShape()), inertia);
                motionState->setRigidBody(body);
            } else {
                body->setMassProps(mass, inertia);
            }
            body->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
            body->updateInertiaTensor();
            break;
        }
    }
    body->setFlags(BT_DISABLE_WORLD_GRAVITY);
    motionState->updateBodyMaterialProperties();

    int16_t group, mask;
    motionState->computeCollisionGroupAndMask(group, mask);
    _dynamicsWorld->addRigidBody(body, group, mask);

    motionState->clearIncomingDirtyFlags();
}

void PhysicsEngine::removeObjects(const VectorOfMotionStates& objects) {
    // first bump and prune contacts for all objects in the list
    for (auto object : objects) {
        bumpAndPruneContacts(object);
    }

    // then remove them
    for (auto object : objects) {
        btRigidBody* body = object->getRigidBody();
        if (body) {
            _dynamicsWorld->removeRigidBody(body);

            // NOTE: setRigidBody() modifies body->m_userPointer so we should clear the MotionState's body BEFORE deleting it.
            object->setRigidBody(nullptr);
            body->setMotionState(nullptr);
            delete body;
        }
    }
}

// Same as above, but takes a Set instead of a Vector.  Should only be called during teardown.
void PhysicsEngine::removeObjects(const SetOfMotionStates& objects) {
    _contactMap.clear();
    for (auto object : objects) {
        btRigidBody* body = object->getRigidBody();
        if (body) {
            _dynamicsWorld->removeRigidBody(body);

            // NOTE: setRigidBody() modifies body->m_userPointer so we should clear the MotionState's body BEFORE deleting it.
            object->setRigidBody(nullptr);
            body->setMotionState(nullptr);
            delete body;
        }
    }
}

void PhysicsEngine::addObjects(const VectorOfMotionStates& objects) {
    for (auto object : objects) {
        addObjectToDynamicsWorld(object);
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
            object->handleEasyChanges(flags);
            object->clearIncomingDirtyFlags();
        }
        if (object->getMotionType() == MOTION_TYPE_STATIC && object->isActive()) {
            _activeStaticBodies.push_back(object->getRigidBody());
        }
    }
    // active static bodies have changed (in an Easy way) and need their Aabbs updated
    // but we've configured Bullet to NOT update them automatically (for improved performance)
    // so we must do it ourselves
    for (size_t i = 0; i < _activeStaticBodies.size(); ++i) {
        _dynamicsWorld->updateSingleAabb(_activeStaticBodies[i]);
    }
    return stillNeedChange;
}

void PhysicsEngine::reinsertObject(ObjectMotionState* object) {
    // remove object from DynamicsWorld
    bumpAndPruneContacts(object);
    btRigidBody* body = object->getRigidBody();
    if (body) {
        _dynamicsWorld->removeRigidBody(body);

        // add it back
        addObjectToDynamicsWorld(object);
    }
}

void PhysicsEngine::removeContacts(ObjectMotionState* motionState) {
    // trigger events for new/existing/old contacts
    ContactMap::iterator contactItr = _contactMap.begin();
    while (contactItr != _contactMap.end()) {
        if (contactItr->first._a == motionState || contactItr->first._b == motionState) {
            contactItr = _contactMap.erase(contactItr);
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

    if (_myAvatarController) {
        BT_PROFILE("avatarController");
        // TODO: move this stuff outside and in front of stepSimulation, because
        // the updateShapeIfNecessary() call needs info from MyAvatar and should
        // be done on the main thread during the pre-simulation stuff
        if (_myAvatarController->needsRemoval()) {
            _myAvatarController->setDynamicsWorld(nullptr);

            // We must remove any existing contacts for the avatar so that any new contacts will have
            // valid data.  MyAvatar's RigidBody is the ONLY one in the simulation that does not yet
            // have a MotionState so we pass nullptr to removeContacts().
            removeContacts(nullptr);
        }
        _myAvatarController->updateShapeIfNecessary();
        if (_myAvatarController->needsAddition()) {
            _myAvatarController->setDynamicsWorld(_dynamicsWorld);
        }
        _myAvatarController->preSimulation();
    }

    auto onSubStep = [this]() {
        updateContactMap();
    };

    int numSubsteps = _dynamicsWorld->stepSimulationWithSubstepCallback(timeStep, PHYSICS_ENGINE_MAX_NUM_SUBSTEPS,
                                                                        PHYSICS_ENGINE_FIXED_SUBSTEP, onSubStep);
    if (numSubsteps > 0) {
        BT_PROFILE("postSimulation");
        _numSubsteps += (uint32_t)numSubsteps;
        ObjectMotionState::setWorldSimulationStep(_numSubsteps);

        if (_myAvatarController) {
            _myAvatarController->postSimulation();
        }

        _hasOutgoingChanges = true;
    }
}

void PhysicsEngine::harvestPerformanceStats() {
    // unfortunately the full context names get too long for our stats presentation format
    //QString contextName = PerformanceTimer::getContextName(); // TODO: how to show full context name?
    QString contextName("...");

    CProfileIterator* profileIterator = CProfileManager::Get_Iterator();
    if (profileIterator) {
        // hunt for stepSimulation context
        profileIterator->First();
        for (int32_t childIndex = 0; !profileIterator->Is_Done(); ++childIndex) {
            if (QString(profileIterator->Get_Current_Name()) == "stepSimulation") {
                profileIterator->Enter_Child(childIndex);
                recursivelyHarvestPerformanceStats(profileIterator, contextName);
                break;
            }
            profileIterator->Next();
        }
    }
}

void PhysicsEngine::recursivelyHarvestPerformanceStats(CProfileIterator* profileIterator, QString contextName) {
    QString parentContextName = contextName + QString("/") + QString(profileIterator->Get_Current_Parent_Name());
    // get the stats for the children
    int32_t numChildren = 0;
    profileIterator->First();
    while (!profileIterator->Is_Done()) {
        QString childContextName = parentContextName + QString("/") + QString(profileIterator->Get_Current_Name());
        uint64_t time = (uint64_t)((btScalar)MSECS_PER_SECOND * profileIterator->Get_Current_Total_Time());
        PerformanceTimer::addTimerRecord(childContextName, time);
        profileIterator->Next();
        ++numChildren;
    }
    // recurse the children
    for (int32_t i = 0; i < numChildren; ++i) {
        profileIterator->Enter_Child(i);
        recursivelyHarvestPerformanceStats(profileIterator, contextName);
    }
    // retreat back to parent
    profileIterator->Enter_Parent();
}

void PhysicsEngine::doOwnershipInfection(const btCollisionObject* objectA, const btCollisionObject* objectB) {
    BT_PROFILE("ownershipInfection");

    const btCollisionObject* characterObject = _myAvatarController ? _myAvatarController->getCollisionObject() : nullptr;

    ObjectMotionState* motionStateA = static_cast<ObjectMotionState*>(objectA->getUserPointer());
    ObjectMotionState* motionStateB = static_cast<ObjectMotionState*>(objectB->getUserPointer());

    if (motionStateB &&
        ((motionStateA && motionStateA->getSimulatorID() == Physics::getSessionUUID() && !objectA->isStaticObject()) ||
         (objectA == characterObject))) {
        // NOTE: we might own the simulation of a kinematic object (A)
        // but we don't claim ownership of kinematic objects (B) based on collisions here.
        if (!objectB->isStaticOrKinematicObject() && motionStateB->getSimulatorID() != Physics::getSessionUUID()) {
            quint8 priorityA = motionStateA ? motionStateA->getSimulationPriority() : PERSONAL_SIMULATION_PRIORITY;
            motionStateB->bump(priorityA);
        }
    } else if (motionStateA &&
               ((motionStateB && motionStateB->getSimulatorID() == Physics::getSessionUUID() && !objectB->isStaticObject()) ||
                (objectB == characterObject))) {
        // SIMILARLY: we might own the simulation of a kinematic object (B)
        // but we don't claim ownership of kinematic objects (A) based on collisions here.
        if (!objectA->isStaticOrKinematicObject() && motionStateA->getSimulatorID() != Physics::getSessionUUID()) {
            quint8 priorityB = motionStateB ? motionStateB->getSimulationPriority() : PERSONAL_SIMULATION_PRIORITY;
            motionStateA->bump(priorityB);
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

            if (!Physics::getSessionUUID().isNull()) {
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

            if (motionStateA) {
                QUuid idA = motionStateA->getObjectID();
                QUuid idB;
                if (motionStateB) {
                    idB = motionStateB->getObjectID();
                }
                glm::vec3 position = bulletToGLM(contact.getPositionWorldOnB()) + _originOffset;
                glm::vec3 penetration = bulletToGLM(contact.distance * contact.normalWorldOnB);
                _collisionEvents.push_back(Collision(type, idA, idB, position, penetration, velocityChange));
            } else if (motionStateB) {
                QUuid idB = motionStateB->getObjectID();
                glm::vec3 position = bulletToGLM(contact.getPositionWorldOnA()) + _originOffset;
                // NOTE: we're flipping the order of A and B (so that the first objectID is never NULL)
                // hence we must negate the penetration.
                glm::vec3 penetration = - bulletToGLM(contact.distance * contact.normalWorldOnB);
                _collisionEvents.push_back(Collision(type, idB, QUuid(), position, penetration, velocityChange));
            }
        }

        if (type == CONTACT_EVENT_TYPE_END) {
            contactItr = _contactMap.erase(contactItr);
        } else {
            ++contactItr;
        }
    }
    return _collisionEvents;
}

const VectorOfMotionStates& PhysicsEngine::getOutgoingChanges() {
    BT_PROFILE("copyOutgoingChanges");
    // Bullet will not deactivate static objects (it doesn't expect them to be active)
    // so we must deactivate them ourselves
    for (size_t i = 0; i < _activeStaticBodies.size(); ++i) {
        _activeStaticBodies[i]->forceActivationState(ISLAND_SLEEPING);
    }
    _activeStaticBodies.clear();
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

void PhysicsEngine::bumpAndPruneContacts(ObjectMotionState* motionState) {
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
    removeContacts(motionState);
}

void PhysicsEngine::setCharacterController(CharacterController* character) {
    if (_myAvatarController != character) {
        if (_myAvatarController) {
            // remove the character from the DynamicsWorld immediately
            _myAvatarController->setDynamicsWorld(nullptr);
            _myAvatarController = nullptr;
        }
        // the character will be added to the DynamicsWorld later
        _myAvatarController = character;
    }
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

void PhysicsEngine::forEachAction(std::function<void(EntityActionPointer)> actor) {
    QHashIterator<QUuid, EntityActionPointer> iter(_objectActions);
    while (iter.hasNext()) {
        iter.next();
        actor(iter.value());
    }
}
