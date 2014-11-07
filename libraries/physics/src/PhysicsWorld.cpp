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

PhysicsWorld::~PhysicsWorld() {
}

void PhysicsWorld::init() {
    _collisionConfig = new btDefaultCollisionConfiguration();
    _collisionDispatcher = new btCollisionDispatcher(_collisionConfig);
    _broadphaseFilter = new btDbvtBroadphase();
    _constraintSolver = new btSequentialImpulseConstraintSolver;
    _dynamicsWorld = new btDiscreteDynamicsWorld(_collisionDispatcher, _broadphaseFilter, _constraintSolver, _collisionConfig);
}

bool PhysicsWorld::addVoxel(const glm::vec3& position, float scale) {
    glm::vec3 halfExtents = glm::vec3(0.5f * scale);
    glm::vec3 center = position + halfExtents;
    PositionHashKey key(center);
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
            transform.setOrigin(btVector3(center.x, center.y, center.z));
            object->setWorldTransform(transform);

            // add to map and world
            _voxels.insert(key, VoxelObject(center, object));
            _dynamicsWorld->addCollisionObject(object);
            return true;
        }
    }
    return false;
}

bool PhysicsWorld::removeVoxel(const glm::vec3& position, float scale) {
    glm::vec3 halfExtents = glm::vec3(0.5f * scale);
    glm::vec3 center = position + halfExtents;
    PositionHashKey key(center);
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

bool PhysicsWorld::addEntity(CustomMotionState* motionState, float mass) {
    assert(motionState);
    ShapeInfo info;
    motionState->computeShapeInfo(info);
    btCollisionShape* shape = _shapeManager.getShape(info);
    if (shape) {
        btVector3 inertia;
        shape->calculateLocalInertia(mass, inertia);
        btRigidBody* body = new btRigidBody(mass, motionState, shape, inertia);
        _dynamicsWorld->addRigidBody(body);
        motionState->_body = body;
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
