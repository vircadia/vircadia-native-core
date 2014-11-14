//
//  CustomMotionState.cpp
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
#include "CustomMotionState.h"

const float MIN_DENSITY = 200.0f;
const float DEFAULT_DENSITY = 1000.0f;
const float MAX_DENSITY = 20000.0f;

const float MIN_VOLUME = 0.001f;
const float DEFAULT_VOLUME = 1.0f;
const float MAX_VOLUME = 1000000.0f;

const float DEFAULT_FRICTION = 0.5f;
const float MAX_FRICTION = 10.0f;

const float DEFAULT_RESTITUTION = 0.0f;

CustomMotionState::CustomMotionState() : 
        _density(DEFAULT_DENSITY), 
        _volume(DEFAULT_VOLUME), 
        _friction(DEFAULT_FRICTION), 
        _restitution(DEFAULT_RESTITUTION), 
        _motionType(MOTION_TYPE_STATIC), 
        _body(NULL) {
}

CustomMotionState::~CustomMotionState() {
    // NOTE: you MUST remove this MotionState from the world before you call the dtor.
    assert(_body == NULL);
}

void CustomMotionState::setDensity(float density) {
    _density = btMax(btMin(fabsf(density), MAX_DENSITY), MIN_DENSITY);
}

void CustomMotionState::setFriction(float friction) {
    _friction = btMax(btMin(fabsf(friction), MAX_FRICTION), 0.0f);
}

void CustomMotionState::setRestitution(float restitution) {
    _restitution = btMax(btMin(fabsf(restitution), 1.0f), 0.0f);
}

void CustomMotionState::setVolume(float volume) {
    _volume = btMax(btMin(fabsf(volume), MAX_VOLUME), MIN_VOLUME);
}

void CustomMotionState::setVelocity(const glm::vec3& velocity) const {
    btVector3 v;
    glmToBullet(velocity, v);
    _body->setLinearVelocity(v);
}

void CustomMotionState::setAngularVelocity(const glm::vec3& velocity) const {
    btVector3 v;
    glmToBullet(velocity, v);
    _body->setAngularVelocity(v);
}

void CustomMotionState::getVelocity(glm::vec3& velocityOut) const {
    bulletToGLM(_body->getLinearVelocity(), velocityOut);
}

void CustomMotionState::getAngularVelocity(glm::vec3& angularVelocityOut) const {
    bulletToGLM(_body->getAngularVelocity(), angularVelocityOut);
}

#endif // USE_BULLET_PHYSICS
