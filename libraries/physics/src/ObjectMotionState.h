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
                EntityItem::DIRTY_MASS | EntityItem::DIRTY_COLLISION_GROUP | EntityItem::DIRTY_MATERIAL);

// These are the set of incoming flags that the PhysicsEngine needs to hear about:
const uint32_t DIRTY_PHYSICS_FLAGS = HARD_DIRTY_PHYSICS_FLAGS | EASY_DIRTY_PHYSICS_FLAGS;

// These are the outgoing flags that the PhysicsEngine can affect:
const uint32_t OUTGOING_DIRTY_PHYSICS_FLAGS = EntityItem::DIRTY_POSITION | EntityItem::DIRTY_VELOCITY;


class OctreeEditPacketSender;

extern const int MAX_NUM_NON_MOVING_UPDATES;

class ObjectMotionState : public btMotionState {
public:
    // The WorldOffset is used to keep the positions of objects in the simulation near the origin, to
    // reduce numerical error when computing vector differences.  In other words: The EntityMotionState 
    // class translates between the simulation-frame and the world-frame as known by the render pipeline, 
    // various object trees, etc.  The EntityMotionState class uses a static "worldOffset" to help in
    // the translations.
    static void setWorldOffset(const glm::vec3& offset);
    static const glm::vec3& getWorldOffset();

    // The WorldSimulationStep is a cached copy of number of SubSteps of the simulation, used for local time measurements.
    static void setWorldSimulationStep(uint32_t step);

    ObjectMotionState();
    ~ObjectMotionState();

    void measureBodyAcceleration();
    void resetMeasuredBodyAcceleration();

    // An EASY update does not require the object to be removed and then reinserted into the PhysicsEngine
    virtual void updateBodyEasy(uint32_t flags, uint32_t frame) = 0;

    virtual void updateBodyMaterialProperties() = 0;
    virtual void updateBodyVelocities() = 0;

    MotionStateType getType() const { return _type; }
    virtual MotionType getMotionType() const { return _motionType; }

    virtual void computeObjectShapeInfo(ShapeInfo& info) = 0;
    virtual float computeObjectMass(const ShapeInfo& shapeInfo) const = 0;

    void setBodyVelocity(const glm::vec3& velocity) const;
    void setBodyAngularVelocity(const glm::vec3& velocity) const;
    void setBodyGravity(const glm::vec3& gravity) const;

    void getVelocity(glm::vec3& velocityOut) const;
    void getAngularVelocity(glm::vec3& angularVelocityOut) const;

    virtual uint32_t getIncomingDirtyFlags() const = 0;
    virtual void clearIncomingDirtyFlags(uint32_t flags) = 0;
    void clearOutgoingPacketFlags(uint32_t flags) { _outgoingPacketFlags &= ~flags; }

    bool doesNotNeedToSendUpdate() const;
    virtual bool shouldSendUpdate(uint32_t simulationStep);
    virtual void sendUpdate(OctreeEditPacketSender* packetSender, uint32_t frame) = 0;

    virtual MotionType computeObjectMotionType() const = 0;

    virtual void updateKinematicState(uint32_t substep) = 0;

    btRigidBody* getRigidBody() const { return _body; }

    bool isKinematic() const { return _isKinematic; }

    void setKinematic(bool kinematic, uint32_t substep);
    virtual void stepKinematicSimulation(quint64 now) = 0;

    virtual bool isMoving() const = 0;

    friend class PhysicsEngine;

    // these are here so we can call into EntityMotionObject with a base-class pointer
    virtual EntityItem* getEntity() const { return NULL; }
    virtual void setShouldClaimSimulationOwnership(bool value) { }
    virtual bool getShouldClaimSimulationOwnership() { return false; }

    // These pure virtual methods must be implemented for each MotionState type
    // and make it possible to implement more complicated methods in this base class.

    virtual float getObjectRestitution() const = 0;
    virtual float getObjectFriction() const = 0;
    virtual float getObjectLinearDamping() const = 0;
    virtual float getObjectAngularDamping() const = 0;
    
    virtual const glm::vec3& getObjectPosition() const = 0;
    virtual const glm::quat& getObjectRotation() const = 0;
    virtual const glm::vec3& getObjectLinearVelocity() const = 0;
    virtual const glm::vec3& getObjectAngularVelocity() const = 0;
    virtual const glm::vec3& getObjectGravity() const = 0;

protected:
    void setRigidBody(btRigidBody* body);

    MotionStateType _type = MOTION_STATE_TYPE_UNKNOWN; // type of MotionState

    MotionType _motionType; // type of motion: KINEMATIC, DYNAMIC, or STATIC

    btRigidBody* _body;

    bool _isKinematic = false;
    uint32_t _lastKinematicSubstep = 0;

    bool _sentMoving;   // true if last update was moving
    int _numNonMovingUpdates; // RELIABLE_SEND_HACK for "not so reliable" resends of packets for non-moving objects

    uint32_t _outgoingPacketFlags;
    uint32_t _sentStep;
    glm::vec3 _sentPosition;    // in simulation-frame (not world-frame)
    glm::quat _sentRotation;;
    glm::vec3 _sentVelocity;
    glm::vec3 _sentAngularVelocity; // radians per second
    glm::vec3 _sentGravity;
    glm::vec3 _sentAcceleration;

    uint32_t _lastSimulationStep;
    glm::vec3 _lastVelocity;
    glm::vec3 _measuredAcceleration;
};

#endif // hifi_ObjectMotionState_h
