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

const float DEFAULT_FRICTION = 0.5f;
const float MAX_FRICTION = 10.0f;

const float DEFAULT_RESTITUTION = 0.5f;

// origin of physics simulation in world frame
glm::vec3 _worldOffset(0.0f);

// static 
void ObjectMotionState::setWorldOffset(const glm::vec3& offset) {
    _worldOffset = offset;
}

// static 
const glm::vec3& ObjectMotionState::getWorldOffset() {
    return _worldOffset;
}


ObjectMotionState::ObjectMotionState() : 
    _friction(DEFAULT_FRICTION), 
    _restitution(DEFAULT_RESTITUTION), 
    _linearDamping(0.0f),
    _angularDamping(0.0f),
    _motionType(MOTION_TYPE_STATIC),
    _body(NULL),
    _sentMoving(false),
    _numNonMovingUpdates(0),
    _outgoingPacketFlags(DIRTY_PHYSICS_FLAGS),
    _sentFrame(0),
    _sentPosition(0.0f),
    _sentRotation(),
    _sentVelocity(0.0f),
    _sentAngularVelocity(0.0f),
    _sentAcceleration(0.0f) {
}

ObjectMotionState::~ObjectMotionState() {
    // NOTE: you MUST remove this MotionState from the world before you call the dtor.
    assert(_body == NULL);
}

void ObjectMotionState::setFriction(float friction) {
    _friction = btMax(btMin(fabsf(friction), MAX_FRICTION), 0.0f);
}

void ObjectMotionState::setRestitution(float restitution) {
    _restitution = btMax(btMin(fabsf(restitution), 1.0f), 0.0f);
}

void ObjectMotionState::setLinearDamping(float damping) {
    _linearDamping = btMax(btMin(fabsf(damping), 1.0f), 0.0f);
}

void ObjectMotionState::setAngularDamping(float damping) {
    _angularDamping = btMax(btMin(fabsf(damping), 1.0f), 0.0f);
}

void ObjectMotionState::setVelocity(const glm::vec3& velocity) const {
    _body->setLinearVelocity(glmToBullet(velocity));
}

void ObjectMotionState::setAngularVelocity(const glm::vec3& velocity) const {
    _body->setAngularVelocity(glmToBullet(velocity));
}

void ObjectMotionState::setGravity(const glm::vec3& gravity) const {
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

bool ObjectMotionState::shouldSendUpdate(uint32_t simulationFrame) {
    assert(_body);
    // if we've never checked before, our _sentFrame will be 0, and we need to initialize our state
    if (_sentFrame == 0) {
        _sentPosition = bulletToGLM(_body->getWorldTransform().getOrigin());
        _sentVelocity = bulletToGLM(_body->getLinearVelocity());
        _sentRotation = bulletToGLM(_body->getWorldTransform().getRotation());
        _sentAngularVelocity = bulletToGLM(_body->getAngularVelocity());
        _sentFrame = simulationFrame;
        return false;
    }
    
    #ifdef WANT_DEBUG
    glm::vec3 wasPosition = _sentPosition;
    glm::quat wasRotation = _sentRotation;
    glm::vec3 wasAngularVelocity = _sentAngularVelocity;
    #endif

    int numFrames = simulationFrame - _sentFrame;
    float dt = (float)(numFrames) * PHYSICS_ENGINE_FIXED_SUBSTEP;
    _sentFrame = simulationFrame;
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

    // NOTE: math in done the simulation-frame, which is NOT necessarily the same as the world-frame 
    // due to _worldOffset.

    // compute position error
    if (glm::length2(_sentVelocity) > 0.0f) {
        _sentVelocity += _sentAcceleration * dt;
        _sentVelocity *= powf(1.0f - _linearDamping, dt);
        _sentPosition += dt * _sentVelocity;
    }

    btTransform worldTrans = _body->getWorldTransform();
    glm::vec3 position = bulletToGLM(worldTrans.getOrigin());
    
    float dx2 = glm::distance2(position, _sentPosition);

    const float MAX_POSITION_ERROR_SQUARED = 0.001f; // 0.001 m^2 ~~> 0.03 m
    if (dx2 > MAX_POSITION_ERROR_SQUARED) {

        #ifdef WANT_DEBUG
            qDebug() << ".... (dx2 > MAX_POSITION_ERROR_SQUARED) ....";
            qDebug() << "wasPosition:" << wasPosition;
            qDebug() << "bullet position:" << position;
            qDebug() << "_sentPosition:" << _sentPosition;
            qDebug() << "dx2:" << dx2;
        #endif

        return true;
    }
    
    if (glm::length2(_sentAngularVelocity) > 0.0f) {
        // compute rotation error
        float attenuation = powf(1.0f - _angularDamping, dt);
        _sentAngularVelocity *= attenuation;
   
        // Bullet caps the effective rotation velocity inside its rotation integration step, therefore 
        // we must integrate with the same algorithm and timestep in order achieve similar results.
        for (int i = 0; i < numFrames; ++i) {
            _sentRotation = glm::normalize(computeBulletRotationStep(_sentAngularVelocity, PHYSICS_ENGINE_FIXED_SUBSTEP) * _sentRotation);
        }
    }
    const float MIN_ROTATION_DOT = 0.99f; // 0.99 dot threshold coresponds to about 16 degrees of slop
    glm::quat actualRotation = bulletToGLM(worldTrans.getRotation());

    #ifdef WANT_DEBUG
        if ((fabsf(glm::dot(actualRotation, _sentRotation)) < MIN_ROTATION_DOT)) {
            qDebug() << ".... ((fabsf(glm::dot(actualRotation, _sentRotation)) < MIN_ROTATION_DOT)) ....";
        
            qDebug() << "wasAngularVelocity:" << wasAngularVelocity;
            qDebug() << "_sentAngularVelocity:" << _sentAngularVelocity;

            qDebug() << "length wasAngularVelocity:" << glm::length(wasAngularVelocity);
            qDebug() << "length _sentAngularVelocity:" << glm::length(_sentAngularVelocity);

            qDebug() << "wasRotation:" << wasRotation;
            qDebug() << "bullet actualRotation:" << actualRotation;
            qDebug() << "_sentRotation:" << _sentRotation;
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
