//
//  PhysicsWorld.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PhysicsWorld.h"
#ifdef USE_BULLET_PHYSICS

PhysicsWorld::~PhysicsWorld() {
}

// virtual
void PhysicsWorld::init() {
    if (!_dynamicsWorld) {
        _collisionConfig = new btDefaultCollisionConfiguration();
        _collisionDispatcher = new btCollisionDispatcher(_collisionConfig);
        _broadphaseFilter = new btDbvtBroadphase();
        _constraintSolver = new btSequentialImpulseConstraintSolver;
        _dynamicsWorld = new btDiscreteDynamicsWorld(_collisionDispatcher, _broadphaseFilter, _constraintSolver, _collisionConfig);

        // TODO: once the initial physics system is working we will set gravity of the world to be zero
        // and each object will have to specify its own local gravity, or we'll set up gravity zones.
        //_dynamicsWorld->setGravity(btVector3(0.0f, 0.0f, 0.0f));
        //
        // GROUND HACK: In the meantime we add a big planar floor to catch falling objects
        // NOTE: we don't care about memory leaking groundShape and groundObject --> 
        // they'll exist until the executable exits.
        const float halfSide = 200.0f;
        const float halfHeight = 10.0f;

        btCollisionShape* groundShape = new btBoxShape(btVector3(halfSide, halfHeight, halfSide));
        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(halfSide, -halfHeight, halfSide));

        btCollisionObject* groundObject = new btCollisionObject();
        groundObject->setCollisionShape(groundShape);
        groundObject->setWorldTransform(groundTransform);
        _dynamicsWorld->addCollisionObject(groundObject);
    }
}

void PhysicsWorld::stepSimulation() {
    const float MAX_TIMESTEP = 1.0f / 30.0f;
    const int MAX_NUM_SUBSTEPS = 2;
    const float FIXED_SUBSTEP = 1.0f / 60.0f;

    float dt = 1.0e-6f * (float)(_clock.getTimeMicroseconds());
    _clock.reset();
    float timeStep = btMin(dt, MAX_TIMESTEP);
    _dynamicsWorld->stepSimulation(timeStep, MAX_NUM_SUBSTEPS, FIXED_SUBSTEP);
}

bool PhysicsWorld::addVoxel(const glm::vec3& position, float scale) {
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

bool PhysicsWorld::removeVoxel(const glm::vec3& position, float scale) {
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

bool PhysicsWorld::addEntity(CustomMotionState* motionState) {
    assert(motionState);
    ShapeInfo info;
    motionState->computeShapeInfo(info);
    btCollisionShape* shape = _shapeManager.getShape(info);
    if (shape) {
        btVector3 inertia(0.0f, 0.0f, 0.0f);
        float mass = 0.0f;
        btRigidBody* body = NULL;
        switch(motionState->_motionType) {
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
                break;
            }
            default: {
                // MOTION_TYPE_STATIC
                body = new btRigidBody(mass, motionState, shape, inertia);
                body->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
                body->updateInertiaTensor();
                motionState->_body = body;
                break;
            }
        }
        body->setRestitution(motionState->_restitution);
        body->setFriction(motionState->_friction);
        _dynamicsWorld->addRigidBody(body);
        return true;
    }
    return false;
}

bool PhysicsWorld::updateEntityMotionType(CustomMotionState* motionState) {
    btRigidBody* body = motionState->_body;
    if (!body) {
        return false;
    }
   
    MotionType oldType = MOTION_TYPE_DYNAMIC;
    if (body->isStaticObject()) {
        oldType = MOTION_TYPE_STATIC;
    } else if (body->isKinematicObject()) {
        oldType = MOTION_TYPE_KINEMATIC;
    }
    MotionType newType = motionState->getMotionType();
    if (oldType != newType) {
        // pull body out of physics engine
        _dynamicsWorld->removeRigidBody(body);

        // update various properties and flags
        switch (newType) {
            case MOTION_TYPE_KINEMATIC: {
                body->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
                body->setActivationState(DISABLE_DEACTIVATION);
                body->updateInertiaTensor();
                break;
            }
            case MOTION_TYPE_DYNAMIC: {
                btVector3 inertia(0.0f, 0.0f, 0.0f);
                float mass = motionState->getMass();
                body->getCollisionShape()->calculateLocalInertia(mass, inertia);
                body->updateInertiaTensor();
                motionState->applyVelocities();
                break;
            }
            default: {
                // MOTION_TYPE_STATIC
                body->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
                body->updateInertiaTensor();
                break;
            }
        }

        // reinsert body into physics engine
        body->setRestitution(motionState->_restitution);
        body->setFriction(motionState->_friction);
        _dynamicsWorld->addRigidBody(body);
        return true;
    }
    return false;
}

bool PhysicsWorld::removeEntity(CustomMotionState* motionState) {
    assert(motionState);
    btRigidBody* body = motionState->_body;
    if (body) {
        const btCollisionShape* shape = body->getCollisionShape();
        ShapeInfo info;
        info.collectInfo(shape);
        _dynamicsWorld->removeRigidBody(body);
        _shapeManager.releaseShape(info);
        delete body;
        motionState->_body = NULL;
        return true;
    }
    return false;
}

bool PhysicsWorld::updateEntityMotionType(CustomMotionState* motionState, MotionType type) {
    // TODO: implement this
    assert(motionState);
    return false;
}

bool PhysicsWorld::updateEntityMassProperties(CustomMotionState* motionState, float mass, const glm::vec3& inertiaEigenValues) {
    // TODO: implement this
    assert(motionState);
    return false;
}

#endif // USE_BULLET_PHYSICS
