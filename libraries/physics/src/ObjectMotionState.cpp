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

#ifdef USE_BULLET_PHYSICS

#include <math.h>

#include "BulletUtil.h"
#include "ObjectMotionState.h"

const float MIN_DENSITY = 200.0f;
const float DEFAULT_DENSITY = 1000.0f;
const float MAX_DENSITY = 20000.0f;

const float MIN_VOLUME = 0.001f;
const float DEFAULT_VOLUME = 1.0f;
const float MAX_VOLUME = 1000000.0f;

const float DEFAULT_FRICTION = 0.5f;
const float MAX_FRICTION = 10.0f;

const float DEFAULT_RESTITUTION = 0.0f;

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
        _density(DEFAULT_DENSITY), 
        _volume(DEFAULT_VOLUME), 
        _friction(DEFAULT_FRICTION), 
        _restitution(DEFAULT_RESTITUTION), 
        _wasInWorld(false),
        _motionType(MOTION_TYPE_STATIC),
        _body(NULL),
        _sentMoving(false),
        _weKnowRecipientHasReceivedNotMoving(false),
        _outgoingPacketFlags(0),
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

void ObjectMotionState::setDensity(float density) {
    _density = btMax(btMin(fabsf(density), MAX_DENSITY), MIN_DENSITY);
}

void ObjectMotionState::setFriction(float friction) {
    _friction = btMax(btMin(fabsf(friction), MAX_FRICTION), 0.0f);
}

void ObjectMotionState::setRestitution(float restitution) {
    _restitution = btMax(btMin(fabsf(restitution), 1.0f), 0.0f);
}

void ObjectMotionState::setVolume(float volume) {
    _volume = btMax(btMin(fabsf(volume), MAX_VOLUME), MIN_VOLUME);
}

void ObjectMotionState::setVelocity(const glm::vec3& velocity) const {
    btVector3 v;
    glmToBullet(velocity, v);
    _body->setLinearVelocity(v);
}

void ObjectMotionState::setAngularVelocity(const glm::vec3& velocity) const {
    btVector3 v;
    glmToBullet(velocity, v);
    _body->setAngularVelocity(v);
}

void ObjectMotionState::setGravity(const glm::vec3& gravity) const {
    btVector3 g;
    glmToBullet(gravity, g);
    _body->setGravity(g);
}

void ObjectMotionState::getVelocity(glm::vec3& velocityOut) const {
    bulletToGLM(_body->getLinearVelocity(), velocityOut);
}

void ObjectMotionState::getAngularVelocity(glm::vec3& angularVelocityOut) const {
    bulletToGLM(_body->getAngularVelocity(), angularVelocityOut);
}

const float FIXED_SUBSTEP = 1.0f / 60.0f;

bool ObjectMotionState::shouldSendUpdate(uint32_t simulationFrame, float subStepRemainder) const {
    assert(_body);
    float dt = (float)(simulationFrame - _sentFrame) * FIXED_SUBSTEP + subStepRemainder;
    const float DEFAULT_UPDATE_PERIOD = 10.0f;
    if (dt > DEFAULT_UPDATE_PERIOD || (_sentMoving && !_body->isActive())) {
        return true;
    }
    // Else we measure the error between current and extrapolated transform (according to expected behavior 
    // of remote EntitySimulation) and return true if the error is significant.

    // NOTE: math in done the simulation-frame, which is NOT necessarily the same as the world-frame 
    // due to _worldOffset.

    // compute position error
    glm::vec3 extrapolatedPosition = _sentPosition + dt * (_sentVelocity + (0.5f * dt) * _sentAcceleration);

    glm::vec3 position;
    btTransform worldTrans = _body->getWorldTransform();
    bulletToGLM(worldTrans.getOrigin(), position);
    
    float dx2 = glm::length2(position - extrapolatedPosition);
    const float MAX_POSITION_ERROR_SQUARED = 0.001f; // 0.001 m^2 ~~> 0.03 m
    if (dx2 > MAX_POSITION_ERROR_SQUARED) {
        return true;
    }

    // compute rotation error
    float spin = glm::length(_sentAngularVelocity);
    glm::quat extrapolatedRotation = _sentRotation;
    const float MIN_SPIN = 1.0e-4f;
    if (spin > MIN_SPIN) {
        glm::vec3 axis = _sentAngularVelocity / spin;
        extrapolatedRotation = glm::angleAxis(dt * spin, axis) * _sentRotation;
    }
    const float MIN_ROTATION_DOT = 0.98f;
    glm::quat actualRotation;
    bulletToGLM(worldTrans.getRotation(), actualRotation);
    return (glm::dot(actualRotation, extrapolatedRotation) < MIN_ROTATION_DOT);
}

#endif // USE_BULLET_PHYSICS
