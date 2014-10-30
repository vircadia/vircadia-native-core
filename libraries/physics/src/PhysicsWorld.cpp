//
//  PhysicsWorld.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PhysicsWorld.h"

PhysicsWorld::~PhysicsWorld() {
}

void PhysicsWorld::init() {
    _collisionConfig = new btDefaultCollisionConfiguration();
    _collisionDispatcher = new btCollisionDispatcher(_collisionConfig);
    _broadphaseFilter = new btDbvtBroadphase();
    _constraintSolver = new btSequentialImpulseConstraintSolver;
    _dynamicsWorld = new btDiscreteDynamicsWorld(_collisionDispatcher, _broadphaseFilter, _constraintSolver, _collisionConfig);
}
