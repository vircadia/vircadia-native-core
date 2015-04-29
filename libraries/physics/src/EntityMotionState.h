//
//  EntityMotionState.h
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2014.11.06
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityMotionState_h
#define hifi_EntityMotionState_h

#include <AACube.h>

#include "ObjectMotionState.h"

class EntityItem;

// From the MotionState's perspective:
//      Inside = physics simulation
//      Outside = external agents (scripts, user interaction, other simulations)

class EntityMotionState : public ObjectMotionState {
public:

    EntityMotionState(btCollisionShape* shape, EntityItem* item);
    virtual ~EntityMotionState();

    /// \return MOTION_TYPE_DYNAMIC or MOTION_TYPE_STATIC based on params set in EntityItem
    virtual MotionType computeObjectMotionType() const;

    virtual void updateKinematicState(uint32_t substep);
    virtual void stepKinematicSimulation(quint64 now);

    virtual bool isMoving() const;

    // this relays incoming position/rotation to the RigidBody
    virtual void getWorldTransform(btTransform& worldTrans) const;

    // this relays outgoing position/rotation to the EntityItem
    virtual void setWorldTransform(const btTransform& worldTrans);

    virtual void computeObjectShapeInfo(ShapeInfo& shapeInfo);
    virtual float computeObjectMass(const ShapeInfo& shapeInfo) const;

    bool doesNotNeedToSendUpdate() const;
    bool remoteSimulationOutOfSync(uint32_t simulationStep);
    bool shouldSendUpdate(uint32_t simulationFrame);
    void sendUpdate(OctreeEditPacketSender* packetSender, uint32_t step);

    void setShouldClaimSimulationOwnership(bool value) { }
    bool getShouldClaimSimulationOwnership() { return false; }

    virtual uint32_t getIncomingDirtyFlags() const;
    virtual void clearIncomingDirtyFlags(uint32_t flags) { _entity->clearDirtyFlags(flags); }

    void incrementAccelerationNearlyGravityCount() { _accelerationNearlyGravityCount++; }
    void resetAccelerationNearlyGravityCount() { _accelerationNearlyGravityCount = 0; }
    quint8 getAccelerationNearlyGravityCount() { return _accelerationNearlyGravityCount; }

    virtual float getObjectRestitution() const { return _entity->getRestitution(); }
    virtual float getObjectFriction() const { return _entity->getFriction(); }
    virtual float getObjectLinearDamping() const { return _entity->getDamping(); }
    virtual float getObjectAngularDamping() const { return _entity->getAngularDamping(); }

    virtual const glm::vec3& getObjectPosition() const { return _entity->getPosition(); }
    virtual const glm::quat& getObjectRotation() const { return _entity->getRotation(); }
    virtual const glm::vec3& getObjectLinearVelocity() const { return _entity->getVelocity(); }
    virtual const glm::vec3& getObjectAngularVelocity() const { return _entity->getAngularVelocity(); }
    virtual const glm::vec3& getObjectGravity() const { return _entity->getGravity(); }

    EntityItem* getEntityItem() const { return _entityItem; }

protected:
    EntityItem* _entity;

    bool _sentMoving;   // true if last update was moving
    int _numNonMovingUpdates; // RELIABLE_SEND_HACK for "not so reliable" resends of packets for non-moving objects

    uint32_t _sentStep;
    glm::vec3 _sentPosition;    // in simulation-frame (not world-frame)
    glm::quat _sentRotation;;
    glm::vec3 _sentVelocity;
    glm::vec3 _sentAngularVelocity; // radians per second
    glm::vec3 _sentGravity;
    glm::vec3 _sentAcceleration;

    quint8 _accelerationNearlyGravityCount;
    bool _shouldClaimSimulationOwnership;
    quint32 _movingStepsWithoutSimulationOwner;
};

#endif // hifi_EntityMotionState_h
