//
//  ObjectMotionState.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.11.05
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <math.h>

#include "BulletUtil.h"
#include "ObjectMotionState.h"
#include "PhysicsEngine.h"
#include "PhysicsHelpers.h"
#include "PhysicsLogging.h"

const float DEFAULT_FRICTION = 0.5f;
const float MAX_FRICTION = 10.0f;

const float DEFAULT_RESTITUTION = 0.5f;

// origin of physics simulation in world-frame
glm::vec3 _worldOffset(0.0f);

// static 
void ObjectMotionState::setWorldOffset(const glm::vec3& offset) {
    _worldOffset = offset;
}

// static 
const glm::vec3& ObjectMotionState::getWorldOffset() {
    return _worldOffset;
}

// static 
uint32_t _worldSimulationStep = 0;
void ObjectMotionState::setWorldSimulationStep(uint32_t step) {
    assert(step > _worldSimulationStep);
    _worldSimulationStep = step;
}

ObjectMotionState::ObjectMotionState(btCollisionShape* shape) :
    _motionType(MOTION_TYPE_STATIC),
    _shape(shape),
    _body(nullptr),
    _mass(0.0f),
    _lastSimulationStep(0),
    _lastVelocity(0.0f),
    _measuredAcceleration(0.0f)
{
    assert(_shape);
}

ObjectMotionState::~ObjectMotionState() {
    // NOTE: you MUST remove this MotionState from the world before you call the dtor.
    assert(_body == nullptr);
}

void ObjectMotionState::measureBodyAcceleration() {
    // try to manually measure the true acceleration of the object
    uint32_t numSubsteps = _worldSimulationStep - _lastSimulationStep;
    if (numSubsteps > 0) {
        float dt = ((float)numSubsteps * PHYSICS_ENGINE_FIXED_SUBSTEP);
        float invDt = 1.0f / dt;
        _lastSimulationStep = _worldSimulationStep;

        // Note: the integration equation for velocity uses damping:   v1 = (v0 + a * dt) * (1 - D)^dt
        // hence the equation for acceleration is: a = (v1 / (1 - D)^dt - v0) / dt
        glm::vec3 velocity = bulletToGLM(_body->getLinearVelocity());
        _measuredAcceleration = (velocity / powf(1.0f - _body->getLinearDamping(), dt) - _lastVelocity) * invDt;
        _lastVelocity = velocity;
    }
}

void ObjectMotionState::resetMeasuredBodyAcceleration() {
    _lastSimulationStep = _worldSimulationStep;
    _lastVelocity = bulletToGLM(_body->getLinearVelocity());
}

void ObjectMotionState::setBodyVelocity(const glm::vec3& velocity) const {
    _body->setLinearVelocity(glmToBullet(velocity));
}

void ObjectMotionState::setBodyAngularVelocity(const glm::vec3& velocity) const {
    _body->setAngularVelocity(glmToBullet(velocity));
}

void ObjectMotionState::setBodyGravity(const glm::vec3& gravity) const {
    _body->setGravity(glmToBullet(gravity));
}

void ObjectMotionState::getVelocity(glm::vec3& velocityOut) const {
    velocityOut = bulletToGLM(_body->getLinearVelocity());
}

void ObjectMotionState::getAngularVelocity(glm::vec3& angularVelocityOut) const {
    angularVelocityOut = bulletToGLM(_body->getAngularVelocity());
}

void ObjectMotionState::setRigidBody(btRigidBody* body) {
    // give the body a (void*) back-pointer to this ObjectMotionState
    if (_body != body) {
        if (_body) {
            _body->setUserPointer(nullptr);
        }
        _body = body;
        if (_body) {
            _body->setUserPointer(this);
        }
    }
}

