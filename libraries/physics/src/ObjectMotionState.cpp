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

ObjectMotionState::ObjectMotionState() : 
    _motionType(MOTION_TYPE_STATIC),
    _body(NULL),
    _sentMoving(false),
    _numNonMovingUpdates(0),
    _outgoingPacketFlags(DIRTY_PHYSICS_FLAGS),
    _sentStep(0),
    _sentPosition(0.0f),
    _sentRotation(),
    _sentVelocity(0.0f),
    _sentAngularVelocity(0.0f),
    _sentGravity(0.0f),
    _sentAcceleration(0.0f),
    _lastSimulationStep(0),
    _lastVelocity(0.0f),
    _measuredAcceleration(0.0f) {
}

ObjectMotionState::~ObjectMotionState() {
    // NOTE: you MUST remove this MotionState from the world before you call the dtor.
    assert(_body == NULL);
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

// RELIABLE_SEND_HACK: until we have truly reliable resends of non-moving updates
// we alwasy resend packets for objects that have stopped moving up to some max limit.
const int MAX_NUM_NON_MOVING_UPDATES = 5;

bool ObjectMotionState::doesNotNeedToSendUpdate() const { 
    return !_body->isActive() && _numNonMovingUpdates > MAX_NUM_NON_MOVING_UPDATES;
}

bool ObjectMotionState::shouldSendUpdate(uint32_t simulationStep) {
    assert(_body);
    // if we've never checked before, our _sentStep will be 0, and we need to initialize our state
    if (_sentStep == 0) {
        btTransform xform = _body->getWorldTransform();
        _sentPosition = bulletToGLM(xform.getOrigin());
        _sentRotation = bulletToGLM(xform.getRotation());
        _sentVelocity = bulletToGLM(_body->getLinearVelocity());
        _sentAngularVelocity = bulletToGLM(_body->getAngularVelocity());
        _sentStep = simulationStep;
        return false;
    }
    
    #ifdef WANT_DEBUG
    glm::vec3 wasPosition = _sentPosition;
    glm::quat wasRotation = _sentRotation;
    glm::vec3 wasAngularVelocity = _sentAngularVelocity;
    #endif

    int numSteps = simulationStep - _sentStep;
    float dt = (float)(numSteps) * PHYSICS_ENGINE_FIXED_SUBSTEP;
    _sentStep = simulationStep;
    bool isActive = _body->isActive();

    if (!isActive) {
        if (_sentMoving) { 
            // this object just went inactive so send an update immediately
            return true;
        } else {
            const float NON_MOVING_UPDATE_PERIOD = 1.0f;
            if (dt > NON_MOVING_UPDATE_PERIOD && _numNonMovingUpdates < MAX_NUM_NON_MOVING_UPDATES) {
                // RELIABLE_SEND_HACK: since we're not yet using a reliable method for non-moving update packets we repeat these
                // at a faster rate than the MAX period above, and only send a limited number of them.
                return true;
            }
        }
    }

    // Else we measure the error between current and extrapolated transform (according to expected behavior 
    // of remote EntitySimulation) and return true if the error is significant.

    // NOTE: math is done in the simulation-frame, which is NOT necessarily the same as the world-frame 
    // due to _worldOffset.

    // compute position error
    if (glm::length2(_sentVelocity) > 0.0f) {
        _sentVelocity += _sentAcceleration * dt;
        _sentVelocity *= powf(1.0f - _body->getLinearDamping(), dt);
        _sentPosition += dt * _sentVelocity;
    }

    btTransform worldTrans = _body->getWorldTransform();
    glm::vec3 position = bulletToGLM(worldTrans.getOrigin());
    
    float dx2 = glm::distance2(position, _sentPosition);

    const float MAX_POSITION_ERROR_SQUARED = 0.001f; // 0.001 m^2 ~~> 0.03 m
    if (dx2 > MAX_POSITION_ERROR_SQUARED) {

        #ifdef WANT_DEBUG
            qCDebug(physics) << ".... (dx2 > MAX_POSITION_ERROR_SQUARED) ....";
            qCDebug(physics) << "wasPosition:" << wasPosition;
            qCDebug(physics) << "bullet position:" << position;
            qCDebug(physics) << "_sentPosition:" << _sentPosition;
            qCDebug(physics) << "dx2:" << dx2;
        #endif

        return true;
    }
    
    if (glm::length2(_sentAngularVelocity) > 0.0f) {
        // compute rotation error
        float attenuation = powf(1.0f - _body->getAngularDamping(), dt);
        _sentAngularVelocity *= attenuation;
   
        // Bullet caps the effective rotation velocity inside its rotation integration step, therefore 
        // we must integrate with the same algorithm and timestep in order achieve similar results.
        for (int i = 0; i < numSteps; ++i) {
            _sentRotation = glm::normalize(computeBulletRotationStep(_sentAngularVelocity, PHYSICS_ENGINE_FIXED_SUBSTEP) * _sentRotation);
        }
    }
    const float MIN_ROTATION_DOT = 0.99f; // 0.99 dot threshold coresponds to about 16 degrees of slop
    glm::quat actualRotation = bulletToGLM(worldTrans.getRotation());

    #ifdef WANT_DEBUG
        if ((fabsf(glm::dot(actualRotation, _sentRotation)) < MIN_ROTATION_DOT)) {
            qCDebug(physics) << ".... ((fabsf(glm::dot(actualRotation, _sentRotation)) < MIN_ROTATION_DOT)) ....";
        
            qCDebug(physics) << "wasAngularVelocity:" << wasAngularVelocity;
            qCDebug(physics) << "_sentAngularVelocity:" << _sentAngularVelocity;

            qCDebug(physics) << "length wasAngularVelocity:" << glm::length(wasAngularVelocity);
            qCDebug(physics) << "length _sentAngularVelocity:" << glm::length(_sentAngularVelocity);

            qCDebug(physics) << "wasRotation:" << wasRotation;
            qCDebug(physics) << "bullet actualRotation:" << actualRotation;
            qCDebug(physics) << "_sentRotation:" << _sentRotation;
        }
    #endif

    return (fabsf(glm::dot(actualRotation, _sentRotation)) < MIN_ROTATION_DOT);
}

void ObjectMotionState::setRigidBody(btRigidBody* body) {
    // give the body a (void*) back-pointer to this ObjectMotionState
    if (_body != body) {
        if (_body) {
            _body->setUserPointer(NULL);
        }
        _body = body;
        if (_body) {
            _body->setUserPointer(this);
        }
    }
}

void ObjectMotionState::setKinematic(bool kinematic, uint32_t substep) {
    _isKinematic = kinematic;
    _lastKinematicSubstep = substep;
}
