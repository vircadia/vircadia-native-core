//
//  PhysicsWorld.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PhysicsWorld_h
#define hifi_PhysicsWorld_h

#ifdef USE_BULLET_PHYSICS

#include <btBulletDynamicsCommon.h>

#include "ShapeManager.h"

class PhysicsWorld {
public:

    PhysicsWorld() : _collisionConfig(NULL), _collisionDispatcher(NULL), 
        _broadphaseFilter(NULL), _constraintSolver(NULL), _dynamicsWorld(NULL) { }

    ~PhysicsWorld();

    void init();
    
protected:
    btDefaultCollisionConfiguration* _collisionConfig;
    btCollisionDispatcher* _collisionDispatcher;
    btBroadphaseInterface* _broadphaseFilter;
    btSequentialImpulseConstraintSolver* _constraintSolver;
    btDiscreteDynamicsWorld* _dynamicsWorld;
    
    ShapeManager _shapeManager;

private:
};


#endif // USE_BULLET_PHYSICS
#endif // hifi_PhysicsWorld_h
