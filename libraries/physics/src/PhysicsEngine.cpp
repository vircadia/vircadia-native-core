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
        _voxels(),
        _frameCount(0) {
}

PhysicsEngine::~PhysicsEngine() {
}

/// \param tree pointer to EntityTree which is stored internally
void PhysicsEngine::setEntityTree(EntityTree* tree) {
    assert(_entityTree == NULL);
    assert(tree);
    _entityTree = tree;
}

void PhysicsEngine::relayIncomingChangesToSimulation() {
    // process incoming changes
    QSet<EntityItem*>::iterator item_itr = _incomingPhysics.begin();
    while (item_itr != _incomingPhysics.end()) {
        EntityItem* entity = *item_itr;
        void* physicsInfo = entity->getPhysicsInfo();
        if (physicsInfo) {
            ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
            uint32_t preOutgoingFlags = motionState->getOutgoingUpdateFlags();
            updateObject(motionState, entity->getUpdateFlags());
            uint32_t postOutgoingFlags = motionState->getOutgoingUpdateFlags();
            if (preOutgoingFlags && !postOutgoingFlags) {
                _outgoingPhysics.remove(entity);
            }
        }
        entity->clearUpdateFlags();
        ++item_itr;
    } 
    _incomingPhysics.clear();
}

/// \param[out] entitiesToDelete list of entities removed from simulation and should be deleted.
void PhysicsEngine::updateEntities(QSet<EntityItem*>& entitiesToDelete) {
    // TODO: Andrew to make this work.
    EnitySimulation::updateEntities(entitiesToDelete);

    item_itr = _outgoingPhysics.begin();
    uint32_t simulationFrame = getSimulationFrame();
    float subStepRemainder = getSubStepRemainder();
    while (item_itr != _outgoingPhysics.end()) {
        EntityItem* entity = *item_itr;
        void* physicsInfo = entity->getPhysicsInfo();
        if (physicsInfo) {
            ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
            if (motionState->shouldSendUpdate(simulationFrame, subStepRemainder)) {
                EntityItemProperties properties = entity->getProperties();
                motionState->pushToProperties(properties);

                /*
                properties.setVelocity(newVelocity);
                properties.setPosition(newPosition);
                properties.setRotation(newRotation);
                properties.setAngularVelocity(newAngularVelocity);
                properties.setLastEdited(now);
                */
        
                EntityItemID id(entity->getID());
                _packetSender->queueEditEntityMessage(PacketTypeEntityAddOrEdit, id, properties);
                ++itemItr;
            } else if (motionState->shouldRemoveFromOutgoingPhysics()) {
                itemItr = _outgoingPhysics.erase(itemItr);
            } else {
                ++itemItr;
            }
        }
    }

    item_itr = _movedEntities.begin();
    AACube domainBounds(glm::vec3(0.0f,0.0f,0.0f), 1.0f);
    while (item_itr != _movedEntities.end()) {
        EntityItem* entity = *item_itr;
        void* physicsInfo = entity->getPhysicsInfo();
        if (physicsInfo) {
            ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
    
            AACube oldCube, newCube;
            motionState->getBoundingCubes(oldCube, newCube);
    
            // check to see if this movement has sent the entity outside of the domain.
            if (!domainBounds.touches(newCube)) {
                //qDebug() << "Entity " << entity->getEntityItemID() << " moved out of domain bounds.";
                entitiesToDelete << entity->getEntityItemID();
                clearEntityState(entity);
            } else if (newCube != oldCube) {
                moveOperator.addEntityToMoveList(entity, oldCube, newCube);
            }
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
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
        removeObject(motionState);
        entity->setPhysicsInfo(NULL);
    }
    _entities.remove(entity);
}

/// \param entity pointer to EntityItem to that may have changed in a way that would affect its simulation
void PhysicsEngine::entityChanged(EntityItem* entity) {
    _incomingPhysics.insert(entity);
}

void PhysicsEngine::clearEntities() {
    // For now we assume this would only be called on shutdown in which case we can just let the memory get lost.
    QSet<EntityItem*>::const_iterator entityItr = _entities.begin();
    for (entityItr = _entities.begin(); entityItr != _entities.end(); ++entityItr) {
        void* physicsInfo = (*entityItr)->getPhysicsInfo();
        if (physicsInfo) {
            ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
            removeObject(motionState);
        }
    }
    _entities.clear();
    _incomingPhysics.clear();
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
}

const float FIXED_SUBSTEP = 1.0f / 60.0f;

void PhysicsEngine::stepSimulation() {
    const int MAX_NUM_SUBSTEPS = 4;
    const float MAX_TIMESTEP = (float)MAX_NUM_SUBSTEPS * FIXED_SUBSTEP;

    float dt = 1.0e-6f * (float)(_clock.getTimeMicroseconds());
    _clock.reset();
    float timeStep = btMin(dt, MAX_TIMESTEP);
    // TODO: Andrew to build a list of outgoingChanges when motionStates are synched, then send the updates out
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

bool PhysicsEngine::updateObject(ObjectMotionState* motionState, uint32_t flags) {
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
void PhysicsEngine::updateObjectHard(btRigidBody* body, ObjectMotionState* motionState, uint32_t flags) {
    MotionType newType = motionState->computeMotionType();

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
void PhysicsEngine::updateObjectEasy(btRigidBody* body, ObjectMotionState* motionState, uint32_t flags) {
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
