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
#ifdef USE_BULLET_PHYSICS

#include "EntityMotionState.h"
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
        _voxels() {
}

PhysicsEngine::~PhysicsEngine() {
}

/// \param tree pointer to EntityTree which is stored internally
void PhysicsEngine::setEntityTree(EntityTree* tree) {
    assert(_entityTree == NULL);
    assert(tree);
    _entityTree = tree;
}

/// \param[out] entitiesToDelete list of entities removed from simulation and should be deleted.
void PhysicsEngine::updateEntities(QSet<EntityItem*>& entitiesToDelete) {
    // relay changes
    QSet<EntityItem*>::iterator item_itr = _changedEntities.begin();
    while (item_itr != _changedEntities.end()) {
        EntityItem* entity = *item_itr;
        void* physicsInfo = entity->getPhysicsInfo();
        if (physicsInfo) {
            CustomMotionState* motionState = static_cast<CustomMotionState*>(physicsInfo);
            updateObject(motionState, entity->getUpdateFlags());
        }
        entity->clearUpdateFlags();
        ++item_itr;
    } 
    _changedEntities.clear();

    // hunt for entities who have expired
    // TODO: make EntityItems use an expiry to make this work faster.
    item_itr = _mortalEntities.begin();
    while (item_itr != _mortalEntities.end()) {
        EntityItem* entity = *item_itr;
        // always check to see if the lifetime has expired, for immortal entities this is always false
        if (entity->lifetimeHasExpired()) {
            qDebug() << "Lifetime has expired for entity:" << entity->getEntityItemID();
            entitiesToDelete.insert(entity);
            // remove entity from the list
            item_itr = _mortalEntities.erase(item_itr);
            _entities.remove(entity);
        } else if (entity->isImmortal()) {
            // remove entity from the list
            item_itr = _mortalEntities.erase(item_itr);
        } else {
            ++item_itr;
        }
    }
    // TODO: check for entities that have exited the world boundaries
}

/// \param entity pointer to EntityItem to add to the simulation
/// \sideeffect the EntityItem::_simulationState member may be updated to indicate membership to internal list
void PhysicsEngine::addEntity(EntityItem* entity) {
    assert(entity);
    void* physicsInfo = entity->getPhysicsInfo();
    if (!physicsInfo) {
        assert(!_entities.contains(entity));
        _entities.insert(entity);
        if (entity->isMortal()) {
            _mortalEntities.insert(entity);
        }
        EntityMotionState* motionState = new EntityMotionState(entity);
        if (addObject(motionState)) {
            entity->setPhysicsInfo(static_cast<void*>(motionState));
        } else {
            // We failed to add the object to the simulation.  Probably because we couldn't create a shape for it.
            delete motionState;
        }
    }
}

/// \param entity pointer to EntityItem to removed from the simulation
/// \sideeffect the EntityItem::_simulationState member may be updated to indicate non-membership to internal list
void PhysicsEngine::removeEntity(EntityItem* entity) {
    assert(entity);
    void* physicsInfo = entity->getPhysicsInfo();
    if (physicsInfo) {
        CustomMotionState* motionState = static_cast<CustomMotionState*>(physicsInfo);
        removeObject(motionState);
        entity->setPhysicsInfo(NULL);
    }
    _entities.remove(entity);
    _mortalEntities.remove(entity);
}

/// \param entity pointer to EntityItem to that may have changed in a way that would affect its simulation
void PhysicsEngine::entityChanged(EntityItem* entity) {
    _changedEntities.insert(entity);
}

void PhysicsEngine::clearEntities() {
    // For now we assume this would only be called on shutdown in which case we can just let the memory get lost.
    QSet<EntityItem*>::const_iterator entityItr = _entities.begin();
    for (entityItr = _entities.begin(); entityItr != _entities.end(); ++entityItr) {
        void* physicsInfo = (*entityItr)->getPhysicsInfo();
        if (physicsInfo) {
            CustomMotionState* motionState = static_cast<CustomMotionState*>(physicsInfo);
            removeObject(motionState);
        }
    }
    _entities.clear();
    _changedEntities.clear();
    _mortalEntities.clear();
}

// virtual
void PhysicsEngine::init() {
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
        
        // GROUND HACK: In the meantime we add a big planar floor to catch falling objects
        // NOTE: we don't care about memory leaking groundShape and groundObject --> 
        // they'll exist until the executable exits.
        const float halfSide = 200.0f;
        const float halfHeight = 1.0f;

        btCollisionShape* groundShape = new btBoxShape(btVector3(halfSide, halfHeight, halfSide));
        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(halfSide, -halfHeight, halfSide));

        btCollisionObject* groundObject = new btCollisionObject();
        groundObject->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
        groundObject->setCollisionShape(groundShape);
        groundObject->setWorldTransform(groundTransform);
        _dynamicsWorld->addCollisionObject(groundObject);
    }
}

void PhysicsEngine::stepSimulation() {
    const float MAX_TIMESTEP = 1.0f / 30.0f;
    const int MAX_NUM_SUBSTEPS = 2;
    const float FIXED_SUBSTEP = 1.0f / 60.0f;

    float dt = 1.0e-6f * (float)(_clock.getTimeMicroseconds());
    _clock.reset();
    float timeStep = btMin(dt, MAX_TIMESTEP);
    _dynamicsWorld->stepSimulation(timeStep, MAX_NUM_SUBSTEPS, FIXED_SUBSTEP);
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

bool PhysicsEngine::addObject(CustomMotionState* motionState) {
    assert(motionState);
    ShapeInfo info;
    motionState->computeShapeInfo(info);
    btCollisionShape* shape = _shapeManager.getShape(info);
    if (shape) {
        btVector3 inertia(0.0f, 0.0f, 0.0f);
        float mass = 0.0f;
        btRigidBody* body = NULL;
        switch(motionState->getMotionType()) {
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

bool PhysicsEngine::removeObject(CustomMotionState* motionState) {
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

bool PhysicsEngine::updateObject(CustomMotionState* motionState, uint32_t flags) {
    btRigidBody* body = motionState->_body;
    if (!body) {
        return false;
    }

    if (flags & PHYSICS_UPDATE_HARD) {
        // a hard update requires the body be pulled out of physics engine, changed, then reinserted
        updateObjectHard(body, motionState, flags);
    } else if (flags & PHYSICS_UPDATE_EASY) {
        // an easy update does not require that the body be pulled out of physics engine
        updateObjectEasy(body, motionState, flags);
    }
    return true;
}

// private
void PhysicsEngine::updateObjectHard(btRigidBody* body, CustomMotionState* motionState, uint32_t flags) {
    MotionType newType = motionState->getMotionType();

    // pull body out of physics engine
    _dynamicsWorld->removeRigidBody(body);

    if (flags & PHYSICS_UPDATE_SHAPE) {
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
        assert(flags & PHYSICS_UPDATE_MASS);
    }
    bool easyUpdate = flags & PHYSICS_UPDATE_EASY;
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
            if (! (flags & PHYSICS_UPDATE_MASS)) {
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
void PhysicsEngine::updateObjectEasy(btRigidBody* body, CustomMotionState* motionState, uint32_t flags) {
    if (flags & PHYSICS_UPDATE_POSITION) {
        btTransform transform;
        motionState->getWorldTransform(transform);
        body->setWorldTransform(transform);
    }
    if (flags & PHYSICS_UPDATE_VELOCITY) {
        motionState->applyVelocities();
        motionState->applyGravity();
    }
    body->setRestitution(motionState->_restitution);
    body->setFriction(motionState->_friction);

    if (flags & PHYSICS_UPDATE_MASS) {
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
