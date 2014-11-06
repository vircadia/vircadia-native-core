//
//  CustomMotionState.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.11.05
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CustomMotionState_h
#define hifi_CustomMotionState_h

#ifdef USE_BULLET_PHYSICS

#include <btBulletDynamicsCommon.h>

#include "ShapeInfo.h"
#include "UUIDHashKey.h"

enum MotionType {
    MOTION_TYPE_STATIC,     // no motion
    MOTION_TYPE_DYNAMIC,    // motion according to physical laws
    MOTION_TYPE_KINEMATIC   // keyframed motion
};

class CustomMotionState : public btMotionState {
public:
    CustomMotionState();

    //// these override methods of the btMotionState base class
    //virtual void getWorldTransform (btTransform &worldTrans) const;
    //virtual void setWorldTransform (const btTransform &worldTrans);

    virtual void computeMassProperties() = 0;
    virtual void getShapeInfo(ShapeInfo& info) = 0;

    bool makeStatic();
    bool makeDynamic();
    bool makeKinematic();

    MotionType getMotionType() const { return _motionType; }

private:
    friend class PhysicsWorld;

    //EntityItem* _entity;
    MotionType _motionType;
    btVector3 _inertiaDiagLocal;
    float _mass;
    btCollisionShape* _shape;
    btCollisionObject* _object;
};

#endif // USE_BULLET_PHYSICS
#endif // hifi_CustomMotionState_h
