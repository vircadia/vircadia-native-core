//
//  PhysicsEntity.cpp
//  libraries/physics/src
//
//  Created by Andrew Meadows 2014.06.11
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PhysicsEntity.h"

PhysicsEntity::PhysicsEntity() : 
    _translation(0.0f), 
    _rotation(), 
    _boundingRadius(0.0f) {
}

PhysicsEntity::~PhysicsEntity() {
}

void PhysicsEntity::setTranslation(const glm::vec3& translation) {
    if (_translation != translation) {
        _translation = translation;
    }
}

void PhysicsEntity::setRotation(const glm::quat& rotation) {
    if (_rotation != rotation) {
        _rotation = rotation;
    }
}   

