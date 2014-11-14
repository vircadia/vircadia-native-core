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
#include <glm/glm.hpp>

#include "ShapeInfo.h"

enum MotionType {
    MOTION_TYPE_STATIC,     // no motion
    MOTION_TYPE_DYNAMIC,    // motion according to physical laws
    MOTION_TYPE_KINEMATIC   // keyframed motion
};

class CustomMotionState : public btMotionState {
public:
    CustomMotionState();
    ~CustomMotionState();

    //// these override methods of the btMotionState base class
    //virtual void getWorldTransform (btTransform &worldTrans) const;
    //virtual void setWorldTransform (const btTransform &worldTrans);

    virtual void applyVelocities() const = 0;

    virtual void computeShapeInfo(ShapeInfo& info) = 0;

    MotionType getMotionType() const { return _motionType; }

    void setDensity(float density);
    void setFriction(float friction);
    void setRestitution(float restitution);
    void setVolume(float volume);

    float getMass() const { return _volume * _density; }

    void setVelocity(const glm::vec3& velocity) const;
    void setAngularVelocity(const glm::vec3& velocity) const;

    void getVelocity(glm::vec3& velocityOut) const;
    void getAngularVelocity(glm::vec3& angularVelocityOut) const;

    friend class PhysicsWorld;
protected:

    float _density;
    float _volume;
    float _friction;
    float _restitution;

    // The data members below have NO setters.  They are only changed by a PhysicsWorld instance.
    MotionType _motionType;
    btRigidBody* _body;
};

#endif // USE_BULLET_PHYSICS
#endif // hifi_CustomMotionState_h
