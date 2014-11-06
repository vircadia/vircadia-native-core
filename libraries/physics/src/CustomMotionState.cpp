//
//  EntityMotionState.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.11.05
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifdef USE_BULLET_PHYSICS

#include "EntityMotionState.h"

EntityMotionState::EntityMotionState() : _motionType(MOTION_TYPE_STATIC), 
    _inertiaDiagLocal(1.0f, 1.0f, 1.0f), _mass(1.0f), 
    _shape(NULL), _object(NULL) {
}

/*
void EntityMotionState::getWorldTransform (btTransform &worldTrans) const {
}

void EntityMotionState::setWorldTransform (const btTransform &worldTrans) {
}

void EntityMotionState::computeMassProperties() {
}

void EntityMotionState::getShapeInfo(ShapeInfo& info) {
}
*/

bool EntityMotionState::makeStatic() {
    if (_motionType == MOTION_TYPE_STATIC) {
        return true;
    }
    if (!_object) {
        _motionType = MOTION_TYPE_STATIC;
        return true;
    }
    return false;
}

bool EntityMotionState::makeDynamic() {
    if (_motionType == MOTION_TYPE_DYNAMIC) {
        return true;
    }
    if (!_object) {
        _motionType = MOTION_TYPE_DYNAMIC;
        return true;
    }
    return false;
}

bool EntityMotionState::makeKinematic() {
    if (_motionType == MOTION_TYPE_KINEMATIC) {
        return true;
    }
    if (!_object) {
        _motionType = MOTION_TYPE_KINEMATIC;
        return true;
    }
    return false;
}

#endif // USE_BULLET_PHYSICS
