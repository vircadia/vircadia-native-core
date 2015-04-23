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

    // The OutgoingEntityQueue is a pointer to a QSet (owned by an EntitySimulation) of EntityItem*'s 
    // that have been changed by the physics simulation.
    // All ObjectMotionState's with outgoing changes put themselves on the list.
    static void setOutgoingEntityList(QSet<EntityItem*>* list);
    static void enqueueOutgoingEntity(EntityItem* entity);

    EntityMotionState() = delete; // prevent compiler from making default ctor
    EntityMotionState(EntityItem* item);
    virtual ~EntityMotionState();

    /// \return MOTION_TYPE_DYNAMIC or MOTION_TYPE_STATIC based on params set in EntityItem
    virtual MotionType computeMotionType() const;

    virtual void updateKinematicState(uint32_t substep);
    virtual void stepKinematicSimulation(quint64 now);

    virtual bool isMoving() const;

    // this relays incoming position/rotation to the RigidBody
    virtual void getWorldTransform(btTransform& worldTrans) const;

    // this relays outgoing position/rotation to the EntityItem
    virtual void setWorldTransform(const btTransform& worldTrans);

    // these relay incoming values to the RigidBody
    virtual void updateObjectEasy(uint32_t flags, uint32_t step);
    virtual void updateObjectVelocities();

    virtual void computeShapeInfo(ShapeInfo& shapeInfo);
    virtual float computeMass(const ShapeInfo& shapeInfo) const;

    virtual bool shouldSendUpdate(uint32_t simulationFrame);
    virtual void sendUpdate(OctreeEditPacketSender* packetSender, uint32_t step);

    virtual uint32_t getIncomingDirtyFlags() const;
    virtual void clearIncomingDirtyFlags(uint32_t flags) { _entity->clearDirtyFlags(flags); }

    void incrementAccelerationNearlyGravityCount() { _accelerationNearlyGravityCount++; }
    void resetAccelerationNearlyGravityCount() { _accelerationNearlyGravityCount = 0; }
    quint8 getAccelerationNearlyGravityCount() { return _accelerationNearlyGravityCount; }

    virtual EntityItem* getEntity() const { return _entity; }
    virtual void setShouldClaimSimulationOwnership(bool value) { _shouldClaimSimulationOwnership = value; }
    virtual bool getShouldClaimSimulationOwnership() { return _shouldClaimSimulationOwnership; }

protected:
    EntityItem* _entity;
    quint8 _accelerationNearlyGravityCount;
    bool _shouldClaimSimulationOwnership;
};

#endif // hifi_EntityMotionState_h
