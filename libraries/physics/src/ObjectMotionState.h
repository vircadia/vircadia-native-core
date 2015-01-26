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

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include <EntityItem.h>

#include "ContactInfo.h"
#include "ShapeInfo.h"

enum MotionType {
    MOTION_TYPE_STATIC,     // no motion
    MOTION_TYPE_DYNAMIC,    // motion according to physical laws
    MOTION_TYPE_KINEMATIC   // keyframed motion
};

enum MotionStateType {
    MOTION_STATE_TYPE_UNKNOWN,
    MOTION_STATE_TYPE_ENTITY,
    MOTION_STATE_TYPE_AVATAR
};

// The update flags trigger two varieties of updates: "hard" which require the body to be pulled 
// and re-added to the physics engine and "easy" which just updates the body properties.
const uint32_t HARD_DIRTY_PHYSICS_FLAGS = (uint32_t)(EntityItem::DIRTY_MOTION_TYPE | EntityItem::DIRTY_SHAPE);
const uint32_t EASY_DIRTY_PHYSICS_FLAGS = (uint32_t)(EntityItem::DIRTY_POSITION | EntityItem::DIRTY_VELOCITY |
                EntityItem::DIRTY_MASS | EntityItem::DIRTY_COLLISION_GROUP);

// These are the set of incoming flags that the PhysicsEngine needs to hear about:
const uint32_t DIRTY_PHYSICS_FLAGS = HARD_DIRTY_PHYSICS_FLAGS | EASY_DIRTY_PHYSICS_FLAGS;

// These are the outgoing flags that the PhysicsEngine can affect:
const uint32_t OUTGOING_DIRTY_PHYSICS_FLAGS = EntityItem::DIRTY_POSITION | EntityItem::DIRTY_VELOCITY;


class OctreeEditPacketSender;

class ObjectMotionState : public btMotionState {
public:
    // The WorldOffset is used to keep the positions of objects in the simulation near the origin, to
    // reduce numerical error when computing vector differences.  In other words: The EntityMotionState 
    // class translates between the simulation-frame and the world-frame as known by the render pipeline, 
    // various object trees, etc.  The EntityMotionState class uses a static "worldOffset" to help in
    // the translations.
    static void setWorldOffset(const glm::vec3& offset);
    static const glm::vec3& getWorldOffset();

    ObjectMotionState();
    ~ObjectMotionState();

    // An EASY update does not require the object to be removed and then reinserted into the PhysicsEngine
    virtual void updateObjectEasy(uint32_t flags, uint32_t frame) = 0;
    virtual void updateObjectVelocities() = 0;

    MotionStateType getType() const { return _type; }
    virtual MotionType getMotionType() const { return _motionType; }

    virtual void computeShapeInfo(ShapeInfo& info) = 0;
    virtual float computeMass(const ShapeInfo& shapeInfo) const = 0;

    void setFriction(float friction);
    void setRestitution(float restitution);
    void setLinearDamping(float damping);
    void setAngularDamping(float damping);

    void setVelocity(const glm::vec3& velocity) const;
    void setAngularVelocity(const glm::vec3& velocity) const;
    void setGravity(const glm::vec3& gravity) const;

    void getVelocity(glm::vec3& velocityOut) const;
    void getAngularVelocity(glm::vec3& angularVelocityOut) const;

    virtual uint32_t getIncomingDirtyFlags() const = 0;
    virtual void clearIncomingDirtyFlags(uint32_t flags) = 0;
    void clearOutgoingPacketFlags(uint32_t flags) { _outgoingPacketFlags &= ~flags; }

    bool doesNotNeedToSendUpdate() const;
    virtual bool shouldSendUpdate(uint32_t simulationFrame);
    virtual void sendUpdate(OctreeEditPacketSender* packetSender, uint32_t frame) = 0;

    virtual MotionType computeMotionType() const = 0;

    virtual void updateKinematicState(uint32_t substep) = 0;

    btRigidBody* getRigidBody() const { return _body; }

    bool isKinematic() const { return _isKinematic; }

    void setKinematic(bool kinematic, uint32_t substep);
    virtual void stepKinematicSimulation(quint64 now) = 0;

    friend class PhysicsEngine;
protected:
    void setRigidBody(btRigidBody* body);

    MotionStateType _type = MOTION_STATE_TYPE_UNKNOWN;

    // TODO: move these materials properties outside of ObjectMotionState
    float _friction;
    float _restitution;
    float _linearDamping;
    float _angularDamping;

    MotionType _motionType;

    btRigidBody* _body;

    bool _isKinematic = false;
    uint32_t _lastKinematicSubstep = 0;

    bool _sentMoving;   // true if last update was moving
    int _numNonMovingUpdates; // RELIABLE_SEND_HACK for "not so reliable" resends of packets for non-moving objects

    uint32_t _outgoingPacketFlags;
    uint32_t _sentFrame;
    glm::vec3 _sentPosition;    // in simulation-frame (not world-frame)
    glm::quat _sentRotation;;
    glm::vec3 _sentVelocity;
    glm::vec3 _sentAngularVelocity; // radians per second
    glm::vec3 _sentAcceleration;
};

#endif // hifi_ObjectMotionState_h
