//
//  MyCharacterController.h
//  interface/src/avatar
//
//  Created by AndrewMeadows 2015.10.21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MyCharacterController.h"

#include <BulletUtil.h>

#include "MyAvatar.h"

// TODO: improve walking up steps
// TODO: make avatars able to walk up and down steps/slopes
// TODO: make avatars stand on steep slope
// TODO: make avatars not snag on low ceilings

MyCharacterController::MyCharacterController(MyAvatar* avatar) {

    assert(avatar);
    _avatar = avatar;
    updateShapeIfNecessary();
}

MyCharacterController::~MyCharacterController() {
}

void MyCharacterController::updateShapeIfNecessary() {
    if (_pendingFlags & PENDING_FLAG_UPDATE_SHAPE) {
        _pendingFlags &= ~PENDING_FLAG_UPDATE_SHAPE;

        if (_radius > 0.0f) {
            // create RigidBody if it doesn't exist
            if (!_rigidBody) {
                // HACK: the avatar collides using convex hull with a collision margin equal to
                // the old capsule radius.  Two points define a capsule and additional points are
                // spread out at chest level to produce a slight taper toward the feet.  This
                // makes the avatar more likely to collide with vertical walls at a higher point
                // and thus less likely to produce a single-point collision manifold below the
                // _maxStepHeight when walking into against vertical surfaces --> fixes a bug
                // where the "walk up steps" feature would allow the avatar to walk up vertical
                // walls.
                const int32_t NUM_POINTS = 6;
                btVector3 points[NUM_POINTS];
                btVector3 xAxis = btVector3(1.0f, 0.0f, 0.0f);
                btVector3 yAxis = btVector3(0.0f, 1.0f, 0.0f);
                btVector3 zAxis = btVector3(0.0f, 0.0f, 1.0f);
                points[0] = _halfHeight * yAxis;
                points[1] = -_halfHeight * yAxis;
                points[2] = (0.75f * _halfHeight) * yAxis - (0.1f * _radius) * zAxis;
                points[3] = (0.75f * _halfHeight) * yAxis + (0.1f * _radius) * zAxis;
                points[4] = (0.75f * _halfHeight) * yAxis - (0.1f * _radius) * xAxis;
                points[5] = (0.75f * _halfHeight) * yAxis + (0.1f * _radius) * xAxis;
                btCollisionShape* shape = new btConvexHullShape(reinterpret_cast<btScalar*>(points), NUM_POINTS);
                shape->setMargin(_radius);

                // HACK: use some simple mass property defaults for now
                const float DEFAULT_AVATAR_MASS = 100.0f;
                const btVector3 DEFAULT_AVATAR_INERTIA_TENSOR(30.0f, 8.0f, 30.0f);

                _rigidBody = new btRigidBody(DEFAULT_AVATAR_MASS, nullptr, shape, DEFAULT_AVATAR_INERTIA_TENSOR);
            } else {
                btCollisionShape* shape = _rigidBody->getCollisionShape();
                if (shape) {
                    delete shape;
                }
                shape = new btCapsuleShape(_radius, 2.0f * _halfHeight);
                _rigidBody->setCollisionShape(shape);
            }

            _rigidBody->setSleepingThresholds(0.0f, 0.0f);
            _rigidBody->setAngularFactor(0.0f);
            _rigidBody->setWorldTransform(btTransform(glmToBullet(_avatar->getOrientation()),
                                                      glmToBullet(_avatar->getPosition())));
            _rigidBody->setDamping(0.0f, 0.0f);
            if (_state == State::Hover) {
                _rigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
            } else {
                _rigidBody->setGravity(_gravity * _currentUp);
            }
            // KINEMATIC_CONTROLLER_HACK
            if (_moveKinematically) {
                _rigidBody->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
            } else {
                _rigidBody->setCollisionFlags(_rigidBody->getCollisionFlags() &
                        ~(btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_STATIC_OBJECT));
            }
        } else {
            // TODO: handle this failure case
        }
    }
}

