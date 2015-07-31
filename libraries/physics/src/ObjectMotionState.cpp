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
uint32_t worldSimulationStep = 0;
void ObjectMotionState::setWorldSimulationStep(uint32_t step) {
    assert(step > worldSimulationStep);
    worldSimulationStep = step;
}

uint32_t ObjectMotionState::getWorldSimulationStep() {
    return worldSimulationStep;
}

// static 
ShapeManager* shapeManager = nullptr;
void ObjectMotionState::setShapeManager(ShapeManager* manager) {
    assert(manager);
    shapeManager = manager;
}

ShapeManager* ObjectMotionState::getShapeManager() {
    assert(shapeManager); // you must properly set shapeManager before calling getShapeManager()
    return shapeManager;
}

ObjectMotionState::ObjectMotionState(btCollisionShape* shape) :
    _motionType(MOTION_TYPE_STATIC),
    _shape(shape),
    _body(nullptr),
    _mass(0.0f),
    _lastKinematicStep(worldSimulationStep)
{
}

ObjectMotionState::~ObjectMotionState() {
    assert(!_body);
    assert(!_shape);
}

void ObjectMotionState::setBodyLinearVelocity(const glm::vec3& velocity) const {
    _body->setLinearVelocity(glmToBullet(velocity));
}

void ObjectMotionState::setBodyAngularVelocity(const glm::vec3& velocity) const {
    _body->setAngularVelocity(glmToBullet(velocity));
}

void ObjectMotionState::setBodyGravity(const glm::vec3& gravity) const {
    _body->setGravity(glmToBullet(gravity));
}

glm::vec3 ObjectMotionState::getBodyLinearVelocity() const {
    // returns the body's velocity unless it is moving too slow in which case returns zero
    btVector3 velocity = _body->getLinearVelocity();

    // NOTE: the threshold to use here relates to the linear displacement threshold (dX) for sending updates
    // to objects that are tracked server-side (e.g. entities which use dX = 2mm).  Hence an object moving 
    // just under this velocity threshold would trigger an update about V/dX times per second.
    const float MIN_LINEAR_SPEED_SQUARED = 0.0036f; // 6 mm/sec
    if (velocity.length2() < MIN_LINEAR_SPEED_SQUARED) {
        velocity *= 0.0f;
    }
    return bulletToGLM(velocity);
}

glm::vec3 ObjectMotionState::getObjectLinearVelocityChange() const {
    return glm::vec3(0.0f);  // Subclasses override where meaningful.
}

glm::vec3 ObjectMotionState::getBodyAngularVelocity() const {
    return bulletToGLM(_body->getAngularVelocity());
}

void ObjectMotionState::releaseShape() {
    if (_shape) {
        shapeManager->releaseShape(_shape);
        _shape = nullptr;
    }
}

void ObjectMotionState::setMotionType(MotionType motionType) {
    _motionType = motionType;
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

void ObjectMotionState::handleEasyChanges(uint32_t flags, PhysicsEngine* engine) {
    if (flags & EntityItem::DIRTY_POSITION) {
        btTransform worldTrans;
        if (flags & EntityItem::DIRTY_ROTATION) {
            worldTrans.setRotation(glmToBullet(getObjectRotation()));
        } else {
            worldTrans = _body->getWorldTransform();
        }
        worldTrans.setOrigin(glmToBullet(getObjectPosition()));
        _body->setWorldTransform(worldTrans);
    } else if (flags & EntityItem::DIRTY_ROTATION) {
        btTransform worldTrans = _body->getWorldTransform();
        worldTrans.setRotation(glmToBullet(getObjectRotation()));
        _body->setWorldTransform(worldTrans);
    }

    if (flags & EntityItem::DIRTY_LINEAR_VELOCITY) {
        _body->setLinearVelocity(glmToBullet(getObjectLinearVelocity()));
        _body->setGravity(glmToBullet(getObjectGravity()));
    }
    if (flags & EntityItem::DIRTY_ANGULAR_VELOCITY) {
        _body->setAngularVelocity(glmToBullet(getObjectAngularVelocity()));
    }

    if (flags & EntityItem::DIRTY_MATERIAL) {
        updateBodyMaterialProperties();
    }

    if (flags & EntityItem::DIRTY_MASS) {
        updateBodyMassProperties();
    }
}

void ObjectMotionState::handleHardAndEasyChanges(uint32_t flags, PhysicsEngine* engine) {
    if (flags & EntityItem::DIRTY_SHAPE) {
        // make sure the new shape is valid
        btCollisionShape* newShape = computeNewShape();
        if (!newShape) {
            qCDebug(physics) << "Warning: failed to generate new shape!";
            // failed to generate new shape! --> keep old shape and remove shape-change flag
            flags &= ~EntityItem::DIRTY_SHAPE;
            // TODO: force this object out of PhysicsEngine rather than just use the old shape
            if ((flags & HARD_DIRTY_PHYSICS_FLAGS) == 0) {
                // no HARD flags remain, so do any EASY changes
                if (flags & EASY_DIRTY_PHYSICS_FLAGS) {
                    handleEasyChanges(flags, engine);
                }
                return;
            }
        }
        getShapeManager()->releaseShape(_shape);
        if (_shape != newShape) {
            _shape = newShape;
            _body->setCollisionShape(_shape);
        } else {
            // huh... the shape didn't actually change, so we clear the DIRTY_SHAPE flag
            flags &= ~EntityItem::DIRTY_SHAPE;
        }
    }
    if (flags & EASY_DIRTY_PHYSICS_FLAGS) {
        handleEasyChanges(flags, engine);
    }
    // it is possible that there are no HARD flags at this point (if DIRTY_SHAPE was removed)
    // so we check again befoe we reinsert:
    if (flags & HARD_DIRTY_PHYSICS_FLAGS) {
        engine->reinsertObject(this);
    }
}

void ObjectMotionState::updateBodyMaterialProperties() {
    _body->setRestitution(getObjectRestitution());
    _body->setFriction(getObjectFriction());
    _body->setDamping(fabsf(btMin(getObjectLinearDamping(), 1.0f)), fabsf(btMin(getObjectAngularDamping(), 1.0f)));
}

void ObjectMotionState::updateBodyVelocities() {
    setBodyLinearVelocity(getObjectLinearVelocity());
    setBodyAngularVelocity(getObjectAngularVelocity());
    setBodyGravity(getObjectGravity());
    _body->setActivationState(ACTIVE_TAG);
}

void ObjectMotionState::updateBodyMassProperties() {
    float mass = getMass();
    btVector3 inertia(0.0f, 0.0f, 0.0f);
    _body->getCollisionShape()->calculateLocalInertia(mass, inertia);
    _body->setMassProps(mass, inertia);
    _body->updateInertiaTensor();
}

