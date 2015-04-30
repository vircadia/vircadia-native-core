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
    return bulletToGLM(_body->getLinearVelocity());
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

void ObjectMotionState::handleEasyChanges(uint32_t flags) {
    if (flags & EntityItem::DIRTY_POSITION) {
        btTransform worldTrans;
        if (flags & EntityItem::DIRTY_ROTATION) {
            worldTrans = getObjectTransform();
        } else {
            worldTrans = _body->getWorldTransform();
            worldTrans.setOrigin(getObjectPosition());
        }
        _body->setWorldTransform(worldTrans);
    } else {
        btTransform worldTrans = _body->getWorldTransform();
        worldTrans.setRotation(getObjectRotation());
        _body->setWorldTransform(worldTrans);
    }

    if (flags & EntityItem::DIRTY_LINEAR_VELOCITY) {
        _body->setLinearVelocity(glmToBullet(getObjectLinearVelocity()));
    }
    if (flags & EntityItem::DIRTY_ANGULAR_VELOCITY) {
        _body->setAngularVelocity(glmToBullet(getObjectAngularVelocity()));
    }

    if (flags & EntityItem::DIRTY_MATERIAL) {
        updateBodyMaterialProperties();
    }

    if (flags & EntityItem::DIRTY_MASS) {
        float mass = getMass();
        btVector3 inertia(0.0f, 0.0f, 0.0f);
        _body->getCollisionShape()->calculateLocalInertia(mass, inertia);
        _body->setMassProps(mass, inertia);
        _body->updateInertiaTensor();
    }

    if (flags & EntityItem::DIRTY_ACTIVATION) {
        _body->activate();
    }
}

void ObjectMotionState::handleHardAndEasyChanges(uint32_t flags, PhysicsEngine* engine) {
    if (flags & EntityItem::DIRTY_SHAPE) {
        // make sure the new shape is valid
        ShapeInfo shapeInfo;
        motionState->computeObjectShapeInfo(shapeInfo);
        btCollisionShape* newShape = getShapeManager()->getShape(shapeInfo);
        if (!newShape) {
            // failed to generate new shape!
            // remove shape-change flag
            flags &= ~EntityItem::DIRTY_SHAPE;
            // TODO: force this object out of PhysicsEngine rather than just use the old shape
            if (flags & HARD_DIRTY_PHYSICS_FLAGS == 0) {
                // no HARD flags remain, so do any EASY changes
                if (flags & EASY_DIRTY_PHYSICS_FLAGS) {
                    handleEasyChanges(flags);
                }
                return;
            }
        }
        engine->removeRigidBody(_body);
        getShapeManager()->removeReference(_shape);
        _shape = newShape;
        _body->setShape(_shape);
        if (flags & EASY_DIRTY_PHYSICS_FLAGS) {
            handleEasyChanges(flags);
        }
        engine->addRigidBody(_body, motionType, mass);
    } else {
        engine->removeRigidBody(_body);
        if (flags & EASY_DIRTY_PHYSICS_FLAGS) {
            handleEasyChanges(flags);
        }
        engine->addRigidBody(_body, motionType, mass);
    }
}

void ObjectMotionState::updateBodyMaterialProperties() {
    _body->setRestitution(getObjectRestitution());
    _body->setFriction(getObjectFriction());
    _body->setDamping(fabsf(btMin(getObjectLinearDamping(), 1.0f)), fabsf(btMin(getObjectAngularDamping(), 1.0f)));
}

void ObjectMotionState::updateBodyVelocities() {
    setBodyVelocity(getObjectLinearVelocity());
    setBodyAngularVelocity(getObjectAngularVelocity();
    setBodyGravity(getObjectGravity();
    _body->setActivationState(ACTIVE_TAG);
}
