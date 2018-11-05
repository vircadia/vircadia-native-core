//
//  PhysicsEngine.cpp
//  libraries/physics/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PhysicsEngine.h"

#include <functional>

#include <QFile>

#include <PerfStat.h>
#include <PhysicsCollisionGroups.h>
#include <Profile.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>

#include "CharacterController.h"
#include "ObjectMotionState.h"
#include "PhysicsHelpers.h"
#include "PhysicsDebugDraw.h"
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
        _physicsDebugDraw.reset(new PhysicsDebugDraw());

        // hook up debug draw renderer
        _dynamicsWorld->setDebugDrawer(_physicsDebugDraw.get());

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

uint32_t PhysicsEngine::getNumSubsteps() const {
    return _dynamicsWorld->getNumSubsteps();
}

int32_t PhysicsEngine::getNumCollisionObjects() const {
    return _dynamicsWorld ? _dynamicsWorld->getNumCollisionObjects() : 0;
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
            const float MIN_DYNAMIC_MASS = 0.01f;
            if (mass != mass || mass < MIN_DYNAMIC_MASS) {
                mass = MIN_DYNAMIC_MASS;
            }
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
            if (motionState->isLocallyOwned()) {
                _activeStaticBodies.insert(body);
            }
            break;
        }
    }
    body->setFlags(BT_DISABLE_WORLD_GRAVITY);
    motionState->updateBodyMaterialProperties();

    int32_t group, mask;
    motionState->computeCollisionGroupAndMask(group, mask);
    _dynamicsWorld->addRigidBody(body, group, mask);

    motionState->clearIncomingDirtyFlags();
}

QList<EntityDynamicPointer> PhysicsEngine::removeDynamicsForBody(btRigidBody* body) {
    // remove dynamics that are attached to this body
    QList<EntityDynamicPointer> removedDynamics;
    QMutableSetIterator<QUuid> i(_objectDynamicsByBody[body]);

    while (i.hasNext()) {
        QUuid dynamicID = i.next();
        if (dynamicID.isNull()) {
            continue;
        }
        EntityDynamicPointer dynamic = _objectDynamics[dynamicID];
        if (!dynamic) {
            continue;
        }
        removeDynamic(dynamicID);
        removedDynamics += dynamic;
    }
    return removedDynamics;
}

void PhysicsEngine::removeObjects(const VectorOfMotionStates& objects) {
    // bump and prune contacts for all objects in the list
    for (auto object : objects) {
        bumpAndPruneContacts(object);
    }

    if (_activeStaticBodies.size() > 0) {
        // _activeStaticBodies was not cleared last frame.
        // The only way to get here is if a static object were moved but we did not actually step the simulation last
        // frame (because the framerate is faster than our physics simulation rate).  When this happens we must scan
        // _activeStaticBodies for objects that were recently deleted so we don't try to access a dangling pointer.
        for (auto object : objects) {
            std::set<btRigidBody*>::iterator itr = _activeStaticBodies.find(object->getRigidBody());
            if (itr != _activeStaticBodies.end()) {
                _activeStaticBodies.erase(itr);
            }
        }
    }

    // remove bodies
    for (auto object : objects) {
        btRigidBody* body = object->getRigidBody();
        if (body) {
            removeDynamicsForBody(body);
            _dynamicsWorld->removeRigidBody(body);

            // NOTE: setRigidBody() modifies body->m_userPointer so we should clear the MotionState's body BEFORE deleting it.
            object->setRigidBody(nullptr);
            body->setMotionState(nullptr);
            delete body;
        }
    }
}

// Same as above, but takes a Set instead of a Vector.  Should only be called during teardown.
void PhysicsEngine::removeSetOfObjects(const SetOfMotionStates& objects) {
    _contactMap.clear();
    for (auto object : objects) {
        btRigidBody* body = object->getRigidBody();
        if (body) {
            removeDynamicsForBody(body);
            _dynamicsWorld->removeRigidBody(body);

            // NOTE: setRigidBody() modifies body->m_userPointer so we should clear the MotionState's body BEFORE deleting it.
            object->setRigidBody(nullptr);
            body->setMotionState(nullptr);
            delete body;
        }
        object->clearIncomingDirtyFlags();
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
            _activeStaticBodies.insert(object->getRigidBody());
        }
    }
    // active static bodies have changed (in an Easy way) and need their Aabbs updated
    // but we've configured Bullet to NOT update them automatically (for improved performance)
    // so we must do it ourselves
    std::set<btRigidBody*>::const_iterator itr = _activeStaticBodies.begin();
    while (itr != _activeStaticBodies.end()) {
        _dynamicsWorld->updateSingleAabb(*itr);
        ++itr;
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

void PhysicsEngine::processTransaction(PhysicsEngine::Transaction& transaction) {
    // removes
    for (auto object : transaction.objectsToRemove) {
        bumpAndPruneContacts(object);
        btRigidBody* body = object->getRigidBody();
        if (body) {
            removeDynamicsForBody(body);
            _dynamicsWorld->removeRigidBody(body);

            // NOTE: setRigidBody() modifies body->m_userPointer so we should clear the MotionState's body BEFORE deleting it.
            object->setRigidBody(nullptr);
            body->setMotionState(nullptr);
            delete body;
        }
        object->clearIncomingDirtyFlags();
    }

    // adds
    for (auto object : transaction.objectsToAdd) {
        addObjectToDynamicsWorld(object);
    }

    // changes
    std::vector<ObjectMotionState*> failedChanges;
    for (auto object : transaction.objectsToChange) {
        uint32_t flags = object->getIncomingDirtyFlags() & DIRTY_PHYSICS_FLAGS;
        if (flags & HARD_DIRTY_PHYSICS_FLAGS) {
            if (object->handleHardAndEasyChanges(flags, this)) {
                object->clearIncomingDirtyFlags();
            } else {
                failedChanges.push_back(object);
            }
        } else if (flags & EASY_DIRTY_PHYSICS_FLAGS) {
            object->handleEasyChanges(flags);
            object->clearIncomingDirtyFlags();
        }
        if (object->getMotionType() == MOTION_TYPE_STATIC && object->isActive()) {
            _activeStaticBodies.insert(object->getRigidBody());
        }
    }
    // activeStaticBodies have changed (in an Easy way) and need their Aabbs updated
    // but we've configured Bullet to NOT update them automatically (for improved performance)
    // so we must do it ourselves
    std::set<btRigidBody*>::const_iterator itr = _activeStaticBodies.begin();
    while (itr != _activeStaticBodies.end()) {
        _dynamicsWorld->updateSingleAabb(*itr);
        ++itr;
    }
    // we replace objectsToChange with any that failed
    transaction.objectsToChange.swap(failedChanges);
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
        DETAILED_PROFILE_RANGE(simulation_physics, "avatarController");
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
        this->updateContactMap();
        this->doOwnershipInfectionForConstraints();
    };

    int numSubsteps = _dynamicsWorld->stepSimulationWithSubstepCallback(timeStep, PHYSICS_ENGINE_MAX_NUM_SUBSTEPS,
                                                                        PHYSICS_ENGINE_FIXED_SUBSTEP, onSubStep);
    if (numSubsteps > 0) {
        BT_PROFILE("postSimulation");
        if (_myAvatarController) {
            _myAvatarController->postSimulation();
        }
        _hasOutgoingChanges = true;
    }

    if (_physicsDebugDraw->getDebugMode()) {
        _dynamicsWorld->debugDrawWorld();
    }
}

class CProfileOperator {
public:
    CProfileOperator() {}
    void recurse(CProfileIterator* itr, QString context) {
        // The context string will get too long if we accumulate it properly
        //QString newContext = context + QString("/") + itr->Get_Current_Parent_Name();
        // so we use this four-character indentation
        QString newContext = context + QString(".../");
        process(itr, newContext);

        // count the children
        int32_t numChildren = 0;
        itr->First();
        while (!itr->Is_Done()) {
            itr->Next();
            ++numChildren;
        }

        // recurse the children
        if (numChildren > 0) {
            // recurse the children
            for (int32_t i = 0; i < numChildren; ++i) {
                itr->Enter_Child(i);
                recurse(itr, newContext);
            }
        }
        // retreat back to parent
        itr->Enter_Parent();
    }
    virtual void process(CProfileIterator*, QString context) = 0;
};

class StatsHarvester : public CProfileOperator {
public:
    StatsHarvester() {}
    void process(CProfileIterator* itr, QString context) override {
            QString name = context + itr->Get_Current_Parent_Name();
            uint64_t time = (uint64_t)((btScalar)MSECS_PER_SECOND * itr->Get_Current_Parent_Total_Time());
            PerformanceTimer::addTimerRecord(name, time);
        };
};

class StatsWriter : public CProfileOperator {
public:
    StatsWriter(QString filename) : _file(filename) {
        _file.open(QFile::WriteOnly);
        if (_file.error() != QFileDevice::NoError) {
            qCDebug(physics) << "unable to open file " << _file.fileName() << " to save stepSimulation() stats";
        }
    }
    ~StatsWriter() {
        _file.close();
    }
    void process(CProfileIterator* itr, QString context) override {
        QString name = context + itr->Get_Current_Parent_Name();
        float time = (btScalar)MSECS_PER_SECOND * itr->Get_Current_Parent_Total_Time();
        if (_file.error() == QFileDevice::NoError) {
            QTextStream s(&_file);
            s << name << " = " << time << "\n";
        }
    }
protected:
    QFile _file;
};

void PhysicsEngine::harvestPerformanceStats() {
    // unfortunately the full context names get too long for our stats presentation format
    //QString contextName = PerformanceTimer::getContextName(); // TODO: how to show full context name?
    QString contextName("...");

    CProfileIterator* itr = CProfileManager::Get_Iterator();
    if (itr) {
        // hunt for stepSimulation context
        itr->First();
        for (int32_t childIndex = 0; !itr->Is_Done(); ++childIndex) {
            if (QString(itr->Get_Current_Name()) == "stepSimulation") {
                itr->Enter_Child(childIndex);
                StatsHarvester harvester;
                harvester.recurse(itr, "physics/");
                break;
            }
            itr->Next();
        }
    }
}

void PhysicsEngine::printPerformanceStatsToFile(const QString& filename) {
    CProfileIterator* itr = CProfileManager::Get_Iterator();
    if (itr) {
        // hunt for stepSimulation context
        itr->First();
        for (int32_t childIndex = 0; !itr->Is_Done(); ++childIndex) {
            if (QString(itr->Get_Current_Name()) == "stepSimulation") {
                itr->Enter_Child(childIndex);
                StatsWriter writer(filename);
                writer.recurse(itr, "");
                break;
            }
            itr->Next();
        }
    }
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
            uint8_t priorityA = motionStateA ? motionStateA->getSimulationPriority() : PERSONAL_SIMULATION_PRIORITY;
            motionStateB->bump(priorityA);
        }
    } else if (motionStateA &&
               ((motionStateB && motionStateB->getSimulatorID() == Physics::getSessionUUID() && !objectB->isStaticObject()) ||
                (objectB == characterObject))) {
        // SIMILARLY: we might own the simulation of a kinematic object (B)
        // but we don't claim ownership of kinematic objects (A) based on collisions here.
        if (!objectA->isStaticOrKinematicObject() && motionStateA->getSimulatorID() != Physics::getSessionUUID()) {
            uint8_t priorityB = motionStateB ? motionStateB->getSimulationPriority() : PERSONAL_SIMULATION_PRIORITY;
            motionStateA->bump(priorityB);
        }
    }
}

void PhysicsEngine::updateContactMap() {
    DETAILED_PROFILE_RANGE(simulation_physics, "updateContactMap");
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

void PhysicsEngine::doOwnershipInfectionForConstraints() {
    BT_PROFILE("ownershipInfectionForConstraints");
    const btCollisionObject* characterObject = _myAvatarController ? _myAvatarController->getCollisionObject() : nullptr;
    foreach(const auto& dynamic, _objectDynamics) {
        if (!dynamic) {
            continue;
        }
        QList<btRigidBody*> bodies = std::static_pointer_cast<ObjectDynamic>(dynamic)->getRigidBodies();
        if (bodies.size() > 1) {
            int32_t numOwned = 0;
            int32_t numStatic = 0;
            uint8_t priority = VOLUNTEER_SIMULATION_PRIORITY;
            foreach(btRigidBody* body, bodies) {
                ObjectMotionState* motionState = static_cast<ObjectMotionState*>(body->getUserPointer());
                if (body->isStaticObject()) {
                    ++numStatic;
                } else if (motionState->getType() == MOTIONSTATE_TYPE_AVATAR) {
                    // we can never take ownership of this constraint
                    numOwned = 0;
                    break;
                } else {
                    if (motionState && motionState->getSimulatorID() == Physics::getSessionUUID()) {
                        priority = glm::max(priority, motionState->getSimulationPriority());
                    } else if (body == characterObject) {
                        priority = glm::max(priority, PERSONAL_SIMULATION_PRIORITY);
                    }
                    numOwned++;
                }
            }

            if (numOwned > 0) {
                if (numOwned + numStatic != bodies.size()) {
                    // we have partial ownership but it isn't complete so we walk each object
                    // and bump the simulation priority to the highest priority we encountered earlier
                    foreach(btRigidBody* body, bodies) {
                        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(body->getUserPointer());
                        if (motionState) {
                            // NOTE: we submit priority+1 because the default behavior of bump() is to actually use priority - 1
                            // and we want all priorities of the objects to be at the SAME level
                            motionState->bump(priority + 1);
                        }
                    }
                }
            }
        }
    }
}

const CollisionEvents& PhysicsEngine::getCollisionEvents() {
    _collisionEvents.clear();

    // scan known contacts and trigger events
    ContactMap::iterator contactItr = _contactMap.begin();

    while (contactItr != _contactMap.end()) {
        ContactInfo& contact = contactItr->second;
        ContactEventType type = contact.computeType(_numContactFrames);
        const btScalar SIGNIFICANT_DEPTH = -0.002f; // penetrations have negative distance
        if (type != CONTACT_EVENT_TYPE_CONTINUE ||
                (contact.distance < SIGNIFICANT_DEPTH &&
                 contact.readyForContinue(_numContactFrames))) {
            ObjectMotionState* motionStateA = static_cast<ObjectMotionState*>(contactItr->first._a);
            ObjectMotionState* motionStateB = static_cast<ObjectMotionState*>(contactItr->first._b);

            // NOTE: the MyAvatar RigidBody is the only object in the simulation that does NOT have a MotionState
            // which means should we ever want to report ALL collision events against the avatar we can
            // modify the logic below.
            //
            // We only create events when at least one of the objects is (or should be) owned in the local simulation.
            if (motionStateA && (motionStateA->isLocallyOwnedOrShouldBe())) {
                QUuid idA = motionStateA->getObjectID();
                QUuid idB;
                if (motionStateB) {
                    idB = motionStateB->getObjectID();
                }
                glm::vec3 position = bulletToGLM(contact.getPositionWorldOnB()) + _originOffset;
                glm::vec3 velocityChange = motionStateA->getObjectLinearVelocityChange() +
                    (motionStateB ? motionStateB->getObjectLinearVelocityChange() : glm::vec3(0.0f));
                glm::vec3 penetration = bulletToGLM(contact.distance * contact.normalWorldOnB);
                _collisionEvents.push_back(Collision(type, idA, idB, position, penetration, velocityChange));
            } else if (motionStateB && (motionStateB->isLocallyOwnedOrShouldBe())) {
                QUuid idB = motionStateB->getObjectID();
                QUuid idA;
                if (motionStateA) {
                    idA = motionStateA->getObjectID();
                }
                glm::vec3 position = bulletToGLM(contact.getPositionWorldOnA()) + _originOffset;
                glm::vec3 velocityChange = motionStateB->getObjectLinearVelocityChange() +
                    (motionStateA ? motionStateA->getObjectLinearVelocityChange() : glm::vec3(0.0f));
                // NOTE: we're flipping the order of A and B (so that the first objectID is never NULL)
                // hence we negate the penetration (because penetration always points from B to A).
                glm::vec3 penetration = - bulletToGLM(contact.distance * contact.normalWorldOnB);
                _collisionEvents.push_back(Collision(type, idB, idA, position, penetration, velocityChange));
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

const VectorOfMotionStates& PhysicsEngine::getChangedMotionStates() {
    BT_PROFILE("copyOutgoingChanges");

    _dynamicsWorld->synchronizeMotionStates();

    // Bullet will not deactivate static objects (it doesn't expect them to be active)
    // so we must deactivate them ourselves
    std::set<btRigidBody*>::const_iterator itr = _activeStaticBodies.begin();
    while (itr != _activeStaticBodies.end()) {
        btRigidBody* body = *itr;
        body->forceActivationState(ISLAND_SLEEPING);
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(body->getUserPointer());
        if (motionState) {
            _dynamicsWorld->addChangedMotionState(motionState);
        }
        ++itr;
    }
    _activeStaticBodies.clear();

    _hasOutgoingChanges = false;
    return _dynamicsWorld->getChangedMotionStates();
}

void PhysicsEngine::dumpStatsIfNecessary() {
    if (_dumpNextStats) {
        _dumpNextStats = false;
        CProfileManager::Increment_Frame_Counter();
        if (_saveNextStats) {
            _saveNextStats = false;
            printPerformanceStatsToFile(_statsFilename);
        }
        CProfileManager::dumpAll();
    }
}

void PhysicsEngine::saveNextPhysicsStats(QString filename) {
    _saveNextStats = true;
    _dumpNextStats = true;
    _statsFilename = filename;
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

EntityDynamicPointer PhysicsEngine::getDynamicByID(const QUuid& dynamicID) const {
    if (_objectDynamics.contains(dynamicID)) {
        return _objectDynamics[dynamicID];
    }
    return nullptr;
}

bool PhysicsEngine::addDynamic(EntityDynamicPointer dynamic) {
    assert(dynamic);

    if (!dynamic->isReadyForAdd()) {
        return false;
    }

    const QUuid& dynamicID = dynamic->getID();
    if (_objectDynamics.contains(dynamicID)) {
        if (_objectDynamics[dynamicID] == dynamic) {
            return true;
        }
        removeDynamic(dynamic->getID());
    }

    bool success { false };
    if (dynamic->isAction()) {
        ObjectAction* objectAction = static_cast<ObjectAction*>(dynamic.get());
        _dynamicsWorld->addAction(objectAction);
        success = true;
    } else if (dynamic->isConstraint()) {
        ObjectConstraint* objectConstraint = static_cast<ObjectConstraint*>(dynamic.get());
        btTypedConstraint* constraint = objectConstraint->getConstraint();
        if (constraint) {
            _dynamicsWorld->addConstraint(constraint);
            success = true;
        } // else perhaps not all the rigid bodies are available, yet
    }

    if (success) {
        _objectDynamics[dynamicID] = dynamic;
        foreach(btRigidBody* rigidBody, std::static_pointer_cast<ObjectDynamic>(dynamic)->getRigidBodies()) {
            _objectDynamicsByBody[rigidBody] += dynamic->getID();
        }
    }
    return success;
}

void PhysicsEngine::removeDynamic(const QUuid dynamicID) {
    if (_objectDynamics.contains(dynamicID)) {
        ObjectDynamicPointer dynamic = std::static_pointer_cast<ObjectDynamic>(_objectDynamics[dynamicID]);
        if (!dynamic) {
            return;
        }
        QList<btRigidBody*> rigidBodies = dynamic->getRigidBodies();
        if (dynamic->isAction()) {
            ObjectAction* objectAction = static_cast<ObjectAction*>(dynamic.get());
            _dynamicsWorld->removeAction(objectAction);
        } else {
            ObjectConstraint* objectConstraint = static_cast<ObjectConstraint*>(dynamic.get());
            btTypedConstraint* constraint = objectConstraint->getConstraint();
            if (constraint) {
                _dynamicsWorld->removeConstraint(constraint);
            } else {
                qCDebug(physics) << "PhysicsEngine::removeDynamic of constraint failed";
            }
        }
        _objectDynamics.remove(dynamicID);
        foreach(btRigidBody* rigidBody, rigidBodies) {
            _objectDynamicsByBody[rigidBody].remove(dynamic->getID());
        }
        dynamic->invalidate();
    }
}

void PhysicsEngine::forEachDynamic(std::function<void(EntityDynamicPointer)> actor) {
    QMutableHashIterator<QUuid, EntityDynamicPointer> iter(_objectDynamics);
    while (iter.hasNext()) {
        iter.next();
        if (iter.value()) {
            actor(iter.value());
        }
    }
}

void PhysicsEngine::setShowBulletWireframe(bool value) {
    int mode = _physicsDebugDraw->getDebugMode();
    if (value) {
        _physicsDebugDraw->setDebugMode(mode | btIDebugDraw::DBG_DrawWireframe);
    } else {
        _physicsDebugDraw->setDebugMode(mode & ~btIDebugDraw::DBG_DrawWireframe);
    }
}

void PhysicsEngine::setShowBulletAABBs(bool value) {
    int mode = _physicsDebugDraw->getDebugMode();
    if (value) {
        _physicsDebugDraw->setDebugMode(mode | btIDebugDraw::DBG_DrawAabb);
    } else {
        _physicsDebugDraw->setDebugMode(mode & ~btIDebugDraw::DBG_DrawAabb);
    }
}

void PhysicsEngine::setShowBulletContactPoints(bool value) {
    int mode = _physicsDebugDraw->getDebugMode();
    if (value) {
        _physicsDebugDraw->setDebugMode(mode | btIDebugDraw::DBG_DrawContactPoints);
    } else {
        _physicsDebugDraw->setDebugMode(mode & ~btIDebugDraw::DBG_DrawContactPoints);
    }
}

void PhysicsEngine::setShowBulletConstraints(bool value) {
    int mode = _physicsDebugDraw->getDebugMode();
    if (value) {
        _physicsDebugDraw->setDebugMode(mode | btIDebugDraw::DBG_DrawConstraints);
    } else {
        _physicsDebugDraw->setDebugMode(mode & ~btIDebugDraw::DBG_DrawConstraints);
    }
}

void PhysicsEngine::setShowBulletConstraintLimits(bool value) {
    int mode = _physicsDebugDraw->getDebugMode();
    if (value) {
        _physicsDebugDraw->setDebugMode(mode | btIDebugDraw::DBG_DrawConstraintLimits);
    } else {
        _physicsDebugDraw->setDebugMode(mode & ~btIDebugDraw::DBG_DrawConstraintLimits);
    }
}

struct AllContactsCallback : public btCollisionWorld::ContactResultCallback {
    AllContactsCallback(int32_t mask, int32_t group, const ShapeInfo& shapeInfo, const Transform& transform, btCollisionObject* myAvatarCollisionObject, float threshold) :
        btCollisionWorld::ContactResultCallback(),
        collisionObject(),
        contacts(),
        myAvatarCollisionObject(myAvatarCollisionObject),
        threshold(threshold) {
        const btCollisionShape* collisionShape = ObjectMotionState::getShapeManager()->getShape(shapeInfo);

        collisionObject.setCollisionShape(const_cast<btCollisionShape*>(collisionShape));

        btTransform bulletTransform;
        bulletTransform.setOrigin(glmToBullet(transform.getTranslation()));
        bulletTransform.setRotation(glmToBullet(transform.getRotation()));

        collisionObject.setWorldTransform(bulletTransform);

        m_collisionFilterMask = mask;
        m_collisionFilterGroup = group;
    }

    ~AllContactsCallback() {
        ObjectMotionState::getShapeManager()->releaseShape(collisionObject.getCollisionShape());
    }

    btCollisionObject collisionObject;
    std::vector<ContactTestResult> contacts;
    btCollisionObject* myAvatarCollisionObject;
    btScalar threshold;

    btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0, int partId0, int index0, const btCollisionObjectWrapper* colObj1, int partId1, int index1) override {
        if (cp.m_distance1 > -threshold) {
            return 0;
        }

        const btCollisionObject* otherBody;
        btVector3 penetrationPoint;
        btVector3 otherPenetrationPoint;
        btVector3 normal;
        if (colObj0->m_collisionObject == &collisionObject) {
            otherBody = colObj1->m_collisionObject;
            penetrationPoint = getWorldPoint(cp.m_localPointB, colObj1->getWorldTransform());
            otherPenetrationPoint = getWorldPoint(cp.m_localPointA, colObj0->getWorldTransform());
            normal = -cp.m_normalWorldOnB;
        } else {
            otherBody = colObj0->m_collisionObject;
            penetrationPoint = getWorldPoint(cp.m_localPointA, colObj0->getWorldTransform());
            otherPenetrationPoint = getWorldPoint(cp.m_localPointB, colObj1->getWorldTransform());
            normal = cp.m_normalWorldOnB;
        }

        // TODO: Give MyAvatar a motion state so we don't have to do this
        if ((m_collisionFilterMask & BULLET_COLLISION_GROUP_MY_AVATAR) && myAvatarCollisionObject && myAvatarCollisionObject == otherBody) {
            contacts.emplace_back(Physics::getSessionUUID(), bulletToGLM(penetrationPoint), bulletToGLM(otherPenetrationPoint), bulletToGLM(normal));
            return 0;
        }

        if (!(otherBody->getInternalType() & btCollisionObject::CO_RIGID_BODY)) {
            return 0;
        }
        const btRigidBody* collisionCandidate = static_cast<const btRigidBody*>(otherBody);

        const btMotionState* motionStateCandidate = collisionCandidate->getMotionState();
        const ObjectMotionState* candidate = dynamic_cast<const ObjectMotionState*>(motionStateCandidate);
        if (!candidate) {
            return 0;
        }

        // This is the correct object type. Add it to the list.
        contacts.emplace_back(candidate->getObjectID(), bulletToGLM(penetrationPoint), bulletToGLM(otherPenetrationPoint), bulletToGLM(normal));

        return 0;
    }

protected:
    static btVector3 getWorldPoint(const btVector3& localPoint, const btTransform& transform) {
        return quatRotate(transform.getRotation(), localPoint) + transform.getOrigin();
    }
};

std::vector<ContactTestResult> PhysicsEngine::contactTest(uint16_t mask, const ShapeInfo& regionShapeInfo, const Transform& regionTransform, uint16_t group, float threshold) const {
    // TODO: Give MyAvatar a motion state so we don't have to do this
    btCollisionObject* myAvatarCollisionObject = nullptr;
    if ((mask & USER_COLLISION_GROUP_MY_AVATAR) && _myAvatarController) {
        myAvatarCollisionObject = _myAvatarController->getCollisionObject();
    }

    auto contactCallback = AllContactsCallback((int32_t)mask, (int32_t)group, regionShapeInfo, regionTransform, myAvatarCollisionObject, threshold);
    _dynamicsWorld->contactTest(&contactCallback.collisionObject, contactCallback);

    return contactCallback.contacts;
}

