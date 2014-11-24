//
//  Physics.cpp
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/glm.hpp>

#include <EntityTree.h>
#include <SharedUtil.h>
#include <ThreadSafeDynamicsWorld.h>

#include "Util.h"
#include "world.h"
#include "Physics.h"

// DynamicsImpl is an implementation of ThreadSafeDynamicsWorld that knows how to lock an EntityTree
class DynamicsImpl : public ThreadSafeDynamicsWorld {
public:
    DynamicsImpl(
        btDispatcher* dispatcher,
        btBroadphaseInterface* pairCache,
        btConstraintSolver* constraintSolver,
        btCollisionConfiguration* collisionConfiguration,
        EntityTree* entities)
        :   ThreadSafeDynamicsWorld(dispatcher, pairCache, constraintSolver, collisionConfiguration), _entities(entities) {
        assert(entities);
    }

    bool tryLock() {
        // wait for lock
        _entities->lockForRead();
        return true;
    }

    void unlock() {
        _entities->unlock();
    }
private:
    EntityTree* _entities;
};

ThreadSafePhysicsEngine::ThreadSafePhysicsEngine(const glm::vec3& offset) : PhysicsEngine(offset) {
}

void ThreadSafePhysicsEngine::initSafe(EntityTree* entities) {
    assert(!_dynamicsWorld); // only call this once
    assert(entities);
    _collisionConfig = new btDefaultCollisionConfiguration();                             
    _collisionDispatcher = new btCollisionDispatcher(_collisionConfig);                   
    _broadphaseFilter = new btDbvtBroadphase();                                           
    _constraintSolver = new btSequentialImpulseConstraintSolver;                          
    // ThreadSafePhysicsEngine gets a DynamicsImpl, which derives from ThreadSafeDynamicsWorld
    _dynamicsWorld = new DynamicsImpl(_collisionDispatcher, _broadphaseFilter, _constraintSolver, _collisionConfig, entities);
}

//
//  Applies static friction:  maxVelocity is the largest velocity for which there
//  there is friction, and strength is the amount of friction force applied to reduce
//  velocity. 
// 
void applyStaticFriction(float deltaTime, glm::vec3& velocity, float maxVelocity, float strength) {
    float v = glm::length(velocity);
    if (v < maxVelocity) {
        velocity *= glm::clamp((1.0f - deltaTime * strength * (1.0f - v / maxVelocity)), 0.0f, 1.0f);
    }
}

//
//  Applies velocity damping, with a strength value for linear and squared velocity damping
//

void applyDamping(float deltaTime, glm::vec3& velocity, float linearStrength, float squaredStrength) {
    if (squaredStrength == 0.0f) {
        velocity *= glm::clamp(1.0f - deltaTime * linearStrength, 0.0f, 1.0f);
    } else {
        velocity *= glm::clamp(1.0f - deltaTime * (linearStrength + glm::length(velocity) * squaredStrength), 0.0f, 1.0f);
    }
}

void applyDampedSpring(float deltaTime, glm::vec3& velocity, glm::vec3& position, glm::vec3& targetPosition, float k, float damping) {
    
}
