//
//  ObjectMotionState.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.11.05
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectMotionState_h
#define hifi_ObjectMotionState_h

#ifdef USE_BULLET_PHYSICS

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <EntityItem.h> // for EntityItem::DIRTY_FOO bitmasks

#include "ShapeInfo.h"

// The update flags trigger two varieties of updates: "hard" which require the body to be pulled 
// and re-added to the physics engine and "easy" which just updates the body properties.
typedef unsigned int uint32_t;
const uint32_t HARD_DIRTY_PHYSICS_FLAGS = (uint32_t)(EntityItem::DIRTY_MOTION_TYPE | EntityItem::DIRTY_SHAPE);
const uint32_t EASY_DIRTY_PHYSICS_FLAGS = (uint32_t)(EntityItem::DIRTY_POSITION | EntityItem::DIRTY_VELOCITY |
                EntityItem::DIRTY_MASS | EntityItem::DIRTY_COLLISION_GROUP);

// These are the set of incoming flags that the PhysicsEngine needs to hear about:
const uint32_t DIRTY_PHYSICS_FLAGS = HARD_DIRTY_PHYSICS_FLAGS | EASY_DIRTY_PHYSICS_FLAGS;

// These are the outgoing flags that the PhysicsEngine can affect:
const uint32_t OUTGOING_DIRTY_PHYSICS_FLAGS = EntityItem::DIRTY_POSITION | EntityItem::DIRTY_VELOCITY;

enum MotionType {
    MOTION_TYPE_STATIC,     // no motion
    MOTION_TYPE_DYNAMIC,    // motion according to physical laws
    MOTION_TYPE_KINEMATIC   // keyframed motion
};

class OctreeEditPacketSender;

class ObjectMotionState : public btMotionState {
public:
    ObjectMotionState();
    ~ObjectMotionState();

    //// these override methods of the btMotionState base class
    //virtual void getWorldTransform (btTransform &worldTrans) const;
    //virtual void setWorldTransform (const btTransform &worldTrans);

    virtual void applyVelocities() const = 0;
    virtual void applyGravity() const = 0;

    virtual void computeShapeInfo(ShapeInfo& info) = 0;

    virtual MotionType getMotionType() const { return _motionType; }

    void setDensity(float density);
    void setFriction(float friction);
    void setRestitution(float restitution);
    void setVolume(float volume);

    float getMass() const { return _volume * _density; }

    void setVelocity(const glm::vec3& velocity) const;
    void setAngularVelocity(const glm::vec3& velocity) const;
    void setGravity(const glm::vec3& gravity) const;

    void getVelocity(glm::vec3& velocityOut) const;
    void getAngularVelocity(glm::vec3& angularVelocityOut) const;

    void clearOutgoingDirtyFlags(uint32_t flags) { _outgoingDirtyFlags &= ~flags; }
    bool isAtRest() const { return !(_body->isActive()) && _weKnowRecipientHasReceivedNotMoving; }
    virtual bool shouldSendUpdate(uint32_t simulationFrame, float subStepRemainder) const;
    virtual void sendUpdate(OctreeEditPacketSender* packetSender) = 0;

    virtual MotionType computeMotionType() const = 0;

    friend class PhysicsEngine;
protected:
    float _density;
    float _volume;
    float _friction;
    float _restitution;
    bool _wasInWorld;
    MotionType _motionType;

    // _body has NO setters -- it is only changed by PhysicsEngine
    btRigidBody* _body;

    bool _sentMoving;   // true if last update was moving
    bool _weKnowRecipientHasReceivedNotMoving;

    uint32_t _outgoingDirtyFlags;
    uint32_t _sentFrame;
    glm::vec3 _sentPosition;
    glm::quat _sentRotation;;
    glm::vec3 _sentVelocity;
    glm::vec3 _sentAngularVelocity;
    glm::vec3 _sentAcceleration;
};

#endif // USE_BULLET_PHYSICS
#endif // hifi_ObjectMotionState_h
