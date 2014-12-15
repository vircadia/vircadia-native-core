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
// TODO DONE: make _incomingChanges to be list of MotionState*'s.
// TODO DONE: make MotionState able to clear incoming flags
// TODO DONE: make MotionState::setWorldTransform() put itself on _incomingChanges list
// TODO DONE: give PhysicsEngine instance an _entityPacketSender
// TODO DONE: make sure code compiles when BULLET is not found
// TODO DONE: make sure incoming and outgoing pipelines are connected
// TODO: test entity updates (second viewer sees physics results from first)
// TODO: provide some sort of "reliable" send for "stopped" update

#include "PhysicsEngine.h"
#ifdef USE_BULLET_PHYSICS

#include "ShapeInfoUtil.h"
#include "ThreadSafeDynamicsWorld.h"

class EntityTree;

PhysicsEngine::PhysicsEngine(const glm::vec3& offset)
    :   _collisionConfig(NULL), 
        _collisionDispatcher(NULL), 
        _broadphaseFilter(NULL), 
        _constraintSolver(NULL), 
        _dynamicsWorld(NULL),
        _originOffset(offset),
        _voxels(),
        _entityPacketSender(NULL),
        _frameCount(0) {
}

PhysicsEngine::~PhysicsEngine() {
}

// begin EntitySimulation overrides
void PhysicsEngine::updateEntitiesInternal(const quint64& now) {
    // NOTE: the grand order of operations is:
    // (1) relay incoming changes
    // (2) step simulation
    // (3) synchronize outgoing motion states
    // (4) send outgoing packets

    // this is step (4)
    QSet<ObjectMotionState*>::iterator stateItr = _outgoingPackets.begin();
    uint32_t frame = getFrameCount();
    float subStepRemainder = getSubStepRemainder();
    while (stateItr != _outgoingPackets.end()) {
        ObjectMotionState* state = *stateItr;
        if (state->doesNotNeedToSendUpdate()) {
            stateItr = _outgoingPackets.erase(stateItr);
        } else if (state->shouldSendUpdate(frame, subStepRemainder)) {
            state->sendUpdate(_entityPacketSender, frame);
            ++stateItr;
        } else {
            ++stateItr;
        }
    }
}

void PhysicsEngine::addEntityInternal(EntityItem* entity) {
    assert(entity);
    void* physicsInfo = entity->getPhysicsInfo();
    if (!physicsInfo) {
        EntityMotionState* motionState = new EntityMotionState(entity);
        if (addObject(motionState)) {
            entity->setPhysicsInfo(static_cast<void*>(motionState));
            _entityMotionStates.insert(motionState);
        } else {
            // We failed to add the entity to the simulation.  Probably because we couldn't create a shape for it.
            qDebug() << "failed to add entity " << entity->getEntityItemID() << " to physics engine";
            delete motionState;
        }
    }
}

void PhysicsEngine::removeEntityInternal(EntityItem* entity) {
    assert(entity);
    void* physicsInfo = entity->getPhysicsInfo();
    if (physicsInfo) {
        EntityMotionState* motionState = static_cast<EntityMotionState*>(physicsInfo);
        removeObject(motionState);
        entity->setPhysicsInfo(NULL);
        _entityMotionStates.remove(motionState);
        _incomingChanges.remove(motionState);
        _outgoingPackets.remove(motionState);
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
    }
    _entityMotionStates.clear();
    _incomingChanges.clear();
    _outgoingPackets.clear();
}
// end EntitySimulation overrides

void PhysicsEngine::relayIncomingChangesToSimulation() {
    // process incoming changes
    QSet<ObjectMotionState*>::iterator stateItr = _incomingChanges.begin();
    while (stateItr != _incomingChanges.end()) {
        ObjectMotionState* motionState = *stateItr;
        uint32_t flags = motionState->getIncomingDirtyFlags() & DIRTY_PHYSICS_FLAGS;

        btRigidBody* body = motionState->_body;
        if (body) {
            if (flags & HARD_DIRTY_PHYSICS_FLAGS) {
                // a HARD update requires the body be pulled out of physics engine, changed, then reinserted
                // but it also handles all EASY changes
                updateObjectHard(body, motionState, flags);
            } else if (flags) {
                // an EASY update does NOT require that the body be pulled out of physics engine
                updateObjectEasy(body, motionState, flags);
            }
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
        ++stateItr;
    }
    _incomingChanges.clear();
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
        _dynamicsWorld = new ThreadSafeDynamicsWorld(_collisionDispatcher, _broadphaseFilter, _constraintSolver, _collisionConfig, _entityTree);

        // default gravity of the world is zero, so each object must specify its own gravity
        // TODO: set up gravity zones
        _dynamicsWorld->setGravity(btVector3(0.0f, 0.0f, 0.0f));
        
        // GROUND HACK: add a big planar floor (and walls for testing) to catch falling objects
        btTransform groundTransform;
        groundTransform.setIdentity();
        for (int i = 0; i < 3; ++i) {
            btVector3 normal(0.0f, 0.0f, 0.0f);
            normal[i] = 1.0f;
            btCollisionShape* plane = new btStaticPlaneShape(normal, 0.0f);

            btCollisionObject* groundObject = new btCollisionObject();
            groundObject->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
            groundObject->setCollisionShape(plane);

            groundObject->setWorldTransform(groundTransform);
            _dynamicsWorld->addCollisionObject(groundObject);
        }
    }

    assert(packetSender);
    _entityPacketSender = packetSender;
    EntityMotionState::setOutgoingEntityList(&_entitiesToBeSorted);
}

const float FIXED_SUBSTEP = 1.0f / 60.0f;

void PhysicsEngine::stepSimulation() {
    // NOTE: the grand order of operations is:
    // (1) relay incoming changes
    // (2) step simulation
    // (3) synchronize outgoing motion states
    // (4) send outgoing packets

    // this is step (1)
    relayIncomingChangesToSimulation();

    const int MAX_NUM_SUBSTEPS = 4;
    const float MAX_TIMESTEP = (float)MAX_NUM_SUBSTEPS * FIXED_SUBSTEP;
    float dt = 1.0e-6f * (float)(_clock.getTimeMicroseconds());
    _clock.reset();
    float timeStep = btMin(dt, MAX_TIMESTEP);

    // steps (2) and (3) are performed by ThreadSafeDynamicsWorld::stepSimulation())
    int numSubSteps = _dynamicsWorld->stepSimulation(timeStep, MAX_NUM_SUBSTEPS, FIXED_SUBSTEP);
    _frameCount += (uint32_t)numSubSteps;
}

bool PhysicsEngine::addVoxel(const glm::vec3& position, float scale) {
    glm::vec3 halfExtents = glm::vec3(0.5f * scale);
    glm::vec3 trueCenter = position + halfExtents;
    PositionHashKey key(trueCenter);
    VoxelObject* proxy = _voxels.find(key);
    if (!proxy) {
        // create a shape
        ShapeInfo info;
        info.setBox(halfExtents);
        btCollisionShape* shape = _shapeManager.getShape(info);

        // NOTE: the shape creation will fail when the size of the voxel is out of range
        if (shape) {
            // create a collisionObject
            btCollisionObject* object = new btCollisionObject();
            object->setCollisionShape(shape);
            btTransform transform;
            transform.setIdentity();
            // we shift the center into the simulation's frame
            glm::vec3 shiftedCenter = (position - _originOffset) + halfExtents;
            transform.setOrigin(btVector3(shiftedCenter.x, shiftedCenter.y, shiftedCenter.z));
            object->setWorldTransform(transform);

            // add to map and world
            _voxels.insert(key, VoxelObject(trueCenter, object));
            _dynamicsWorld->addCollisionObject(object);
            return true;
        }
    }
    return false;
}

bool PhysicsEngine::removeVoxel(const glm::vec3& position, float scale) {
    glm::vec3 halfExtents = glm::vec3(0.5f * scale);
    glm::vec3 trueCenter = position + halfExtents;
    PositionHashKey key(trueCenter);
    VoxelObject* proxy = _voxels.find(key);
    if (proxy) {
        // remove from world
        assert(proxy->_object);
        _dynamicsWorld->removeCollisionObject(proxy->_object);

        // release shape
        ShapeInfo info;
        info.setBox(halfExtents);
        bool released = _shapeManager.releaseShape(info);
        assert(released);

        // delete object and remove from voxel map
        delete proxy->_object;
        _voxels.remove(key);
        return true;
    }
    return false;
}

// Bullet collision flags are as follows:
// CF_STATIC_OBJECT= 1,
// CF_KINEMATIC_OBJECT= 2,
// CF_NO_CONTACT_RESPONSE = 4,
// CF_CUSTOM_MATERIAL_CALLBACK = 8,//this allows per-triangle material (friction/restitution)
// CF_CHARACTER_OBJECT = 16,
// CF_DISABLE_VISUALIZE_OBJECT = 32, //disable debug drawing
// CF_DISABLE_SPU_COLLISION_PROCESSING = 64//disable parallel/SPU processing

bool PhysicsEngine::addObject(ObjectMotionState* motionState) {
    assert(motionState);
    ShapeInfo info;
    motionState->computeShapeInfo(info);
    btCollisionShape* shape = _shapeManager.getShape(info);
    if (shape) {
        btVector3 inertia(0.0f, 0.0f, 0.0f);
        float mass = 0.0f;
        btRigidBody* body = NULL;
        switch(motionState->computeMotionType()) {
            case MOTION_TYPE_KINEMATIC: {
                body = new btRigidBody(mass, motionState, shape, inertia);
                body->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
                body->setActivationState(DISABLE_DEACTIVATION);
                body->updateInertiaTensor();
                motionState->_body = body;
                break;
            }
            case MOTION_TYPE_DYNAMIC: {
                mass = motionState->getMass();
                shape->calculateLocalInertia(mass, inertia);
                body = new btRigidBody(mass, motionState, shape, inertia);
                body->updateInertiaTensor();
                motionState->_body = body;
                motionState->applyVelocities();
                motionState->applyGravity();
                break;
            }
            case MOTION_TYPE_STATIC:
            default: {
                body = new btRigidBody(mass, motionState, shape, inertia);
                body->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
                body->updateInertiaTensor();
                motionState->_body = body;
                break;
            }
        }
        // wtf?
        body->setFlags(BT_DISABLE_WORLD_GRAVITY);
        body->setRestitution(motionState->_restitution);
        body->setFriction(motionState->_friction);
        _dynamicsWorld->addRigidBody(body);
        return true;
    }
    return false;
}

bool PhysicsEngine::removeObject(ObjectMotionState* motionState) {
    assert(motionState);
    btRigidBody* body = motionState->_body;
    if (body) {
        const btCollisionShape* shape = body->getCollisionShape();
        ShapeInfo info;
        ShapeInfoUtil::collectInfoFromShape(shape, info);
        _dynamicsWorld->removeRigidBody(body);
        _shapeManager.releaseShape(info);
        delete body;
        motionState->_body = NULL;
        return true;
    }
    return false;
}

// private
void PhysicsEngine::updateObjectHard(btRigidBody* body, ObjectMotionState* motionState, uint32_t flags) {
    MotionType newType = motionState->computeMotionType();

    // pull body out of physics engine
    _dynamicsWorld->removeRigidBody(body);

    if (flags & EntityItem::DIRTY_SHAPE) {
        btCollisionShape* oldShape = body->getCollisionShape();
        ShapeInfo info;
        motionState->computeShapeInfo(info);
        btCollisionShape* newShape = _shapeManager.getShape(info);
        if (newShape != oldShape) {
            body->setCollisionShape(newShape);
            _shapeManager.releaseShape(oldShape);
        } else {
            // whoops, shape hasn't changed after all so we must release the reference
            // that was created when looking it up
            _shapeManager.releaseShape(newShape);
        }
        // MASS bit should be set whenever SHAPE is set
        assert(flags & EntityItem::DIRTY_MASS);
    }
    bool easyUpdate = flags & EASY_DIRTY_PHYSICS_FLAGS;
    if (easyUpdate) {
        updateObjectEasy(body, motionState, flags);
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
            break;
        }
        case MOTION_TYPE_DYNAMIC: {
            int collisionFlags = body->getCollisionFlags() & ~(btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_STATIC_OBJECT);
            body->setCollisionFlags(collisionFlags);
            if (! (flags & EntityItem::DIRTY_MASS)) {
                // always update mass properties when going dynamic (unless it's already been done)
                btVector3 inertia(0.0f, 0.0f, 0.0f);
                float mass = motionState->getMass();
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

    // reinsert body into physics engine
    _dynamicsWorld->addRigidBody(body);

    body->activate();
}

// private
void PhysicsEngine::updateObjectEasy(btRigidBody* body, ObjectMotionState* motionState, uint32_t flags) {
    if (flags & EntityItem::DIRTY_POSITION) {
        btTransform transform;
        motionState->getWorldTransform(transform);
        body->setWorldTransform(transform);
    }
    if (flags & EntityItem::DIRTY_VELOCITY) {
        motionState->applyVelocities();
        motionState->applyGravity();
    }
    body->setRestitution(motionState->_restitution);
    body->setFriction(motionState->_friction);

    if (flags & EntityItem::DIRTY_MASS) {
        float mass = motionState->getMass();
        btVector3 inertia(0.0f, 0.0f, 0.0f);
        body->getCollisionShape()->calculateLocalInertia(mass, inertia);
        body->setMassProps(mass, inertia);
        body->updateInertiaTensor();
    }
    body->activate();

    // TODO: support collision groups
};

#endif // USE_BULLET_PHYSICS
