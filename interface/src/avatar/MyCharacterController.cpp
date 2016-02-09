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

        // compute new dimensions from avatar's bounding box
        float x = _boxScale.x;
        float z = _boxScale.z;
        _radius = 0.5f * sqrtf(0.5f * (x * x + z * z));
        _halfHeight = 0.5f * _boxScale.y - _radius;
        float MIN_HALF_HEIGHT = 0.1f;
        if (_halfHeight < MIN_HALF_HEIGHT) {
            _halfHeight = MIN_HALF_HEIGHT;
        }
        // NOTE: _shapeLocalOffset is already computed

        if (_radius > 0.0f) {
            // HACK: use some simple mass property defaults for now
            float mass = 100.0f;
            btVector3 inertia(30.0f, 8.0f, 30.0f);

            // create RigidBody if it doesn't exist
            if (!_rigidBody) {
                btCollisionShape* shape = new btCapsuleShape(_radius, 2.0f * _halfHeight);
                _rigidBody = new btRigidBody(mass, nullptr, shape, inertia);
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
                _rigidBody->setGravity(DEFAULT_CHARACTER_GRAVITY * _currentUp);
            }
            //_rigidBody->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
        } else {
            // TODO: handle this failure case
        }
    }
}

