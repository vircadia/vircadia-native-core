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

#include <EntityItem.h>
#include <EntityTypes.h>
#include <AACube.h>
#include <workload/Region.h>

#include "ObjectMotionState.h"


// From the MotionState's perspective:
//      Inside = physics simulation
//      Outside = external agents (scripts, user interaction, other simulations)

class EntityMotionState : public ObjectMotionState {
public:
    enum class OwnershipState {
        NotLocallyOwned = 0,
        PendingBid,
        LocallyOwned,
        Unownable
    };

    EntityMotionState() = delete;
    EntityMotionState(btCollisionShape* shape, EntityItemPointer item);
    virtual ~EntityMotionState();

    void handleDeactivation();
    virtual void handleEasyChanges(uint32_t& flags) override;

    /// \return PhysicsMotionType based on params set in EntityItem
    virtual PhysicsMotionType computePhysicsMotionType() const override;

    virtual bool isMoving() const override;

    // this relays incoming position/rotation to the RigidBody
    virtual void getWorldTransform(btTransform& worldTrans) const override;

    // this relays outgoing position/rotation to the EntityItem
    virtual void setWorldTransform(const btTransform& worldTrans) override;

    bool shouldSendUpdate(uint32_t simulationStep);
    void sendBid(OctreeEditPacketSender* packetSender, uint32_t step);
    void sendUpdate(OctreeEditPacketSender* packetSender, uint32_t step);

    virtual uint32_t getIncomingDirtyFlags() const override;
    virtual void clearIncomingDirtyFlags(uint32_t mask = DIRTY_PHYSICS_FLAGS) override;

    virtual float getObjectRestitution() const override { return _entity->getRestitution(); }
    virtual float getObjectFriction() const override { return _entity->getFriction(); }
    virtual float getObjectLinearDamping() const override { return _entity->getDamping(); }
    virtual float getObjectAngularDamping() const override { return _entity->getAngularDamping(); }

    virtual glm::vec3 getObjectPosition() const override { return _entity->getWorldPosition() - ObjectMotionState::getWorldOffset(); }
    virtual glm::quat getObjectRotation() const override { return _entity->getWorldOrientation(); }
    virtual glm::vec3 getObjectLinearVelocity() const override { return _entity->getWorldVelocity(); }
    virtual glm::vec3 getObjectAngularVelocity() const override { return _entity->getWorldAngularVelocity(); }
    virtual glm::vec3 getObjectGravity() const override { return _entity->getGravity(); }
    virtual glm::vec3 getObjectLinearVelocityChange() const override;

    virtual const QUuid getObjectID() const override { return _entity->getID(); }

    virtual uint8_t getSimulationPriority() const override;
    virtual QUuid getSimulatorID() const override;
    virtual void bump(uint8_t priority) override;

    // getEntity() returns a smart-pointer by reference because it is only ever used
    // to insert into lists of smart pointers, and the lists will make their own copies
    const EntityItemPointer& getEntity() const { return _entity; }

    void resetMeasuredBodyAcceleration();
    void measureBodyAcceleration();

    virtual QString getName() const override;
    ShapeType getShapeType() const override { return _entity->getShapeType(); }

    virtual void computeCollisionGroupAndMask(int32_t& group, int32_t& mask) const override;

    bool shouldSendBid() const;

    bool isLocallyOwned() const override;
    bool isLocallyOwnedOrShouldBe() const override; // aka shouldEmitCollisionEvents()

    friend class PhysicalEntitySimulation;
    OwnershipState getOwnershipState() const { return _ownershipState; }

    void setRegion(uint8_t region);
    void saveKinematicState(btScalar timeStep) override;

protected:
    void setRigidBody(btRigidBody* body) override;

    uint8_t computeFinalBidPriority() const;
    void updateSendVelocities();
    uint64_t getNextBidExpiry() const { return _nextBidExpiry; }
    void initForBid();
    void initForOwned();
    void clearOwnershipState();
    void updateServerPhysicsVariables();
    bool remoteSimulationOutOfSync(uint32_t simulationStep);

    void slaveBidPriority(); // computeNewBidPriority() with value stored in _entity

    void clearObjectVelocities() const;

    bool isInPhysicsSimulation() const { return _body != nullptr; }
    bool shouldBeInPhysicsSimulation() const;
    void setMotionType(PhysicsMotionType motionType) override;

    // EntityMotionState keeps a SharedPointer to its EntityItem which is only set in the CTOR
    // and is only cleared in the DTOR
    EntityItemPointer _entity;

    // These "_serverFoo" variables represent what we think the server knows.
    // They are used in two different modes:
    //
    // (1) For remotely owned simulation: we store the last values recieved from the server.
    //     When the body comes to rest and goes inactive we slam its final transforms to agree with the last server
    //     update. This to reduce state synchronization errors when the local simulation deviated from remote.
    //
    // (2) For locally owned simulation: we store the last values sent to the server, integrated forward over time
    //     according to how we think the server doing it.  We calculate the error between the true local transform
    //     and the remote to decide whether to send another update or not.
    //
    glm::vec3 _serverPosition;    // in simulation-frame (not world-frame)
    glm::quat _serverRotation;
    glm::vec3 _serverVelocity;
    glm::vec3 _serverAngularVelocity; // radians per second
    glm::vec3 _serverGravity;
    glm::vec3 _serverAcceleration;
    QByteArray _serverActionData;

    glm::vec3 _lastVelocity;
    glm::vec3 _measuredAcceleration;
    quint64 _nextBidExpiry { 0 };

    float _measuredDeltaTime;
    uint32_t _lastMeasureStep;
    uint32_t _lastStep; // last step of server extrapolation

    OwnershipState _ownershipState { OwnershipState::NotLocallyOwned };
    uint8_t _loopsWithoutOwner;
    mutable uint8_t _accelerationNearlyGravityCount;
    uint8_t _numInactiveUpdates { 1 };
    uint8_t _bumpedPriority { 0 }; // the target simulation priority according to collision history
    uint8_t _region { workload::Region::INVALID };

    bool isServerlessMode();
};

#endif // hifi_EntityMotionState_h
