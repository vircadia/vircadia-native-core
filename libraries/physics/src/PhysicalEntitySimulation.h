//
//  PhysicalEntitySimulation.h
//  libraries/physics/src
//
//  Created by Andrew Meadows 2015.04.27
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PhysicalEntitySimulation_h
#define hifi_PhysicalEntitySimulation_h

#include <stdint.h>
#include <map>
#include <set>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <EntityDynamicInterface.h>
#include <EntityItem.h>
#include <EntitySimulation.h>
#include <workload/Space.h>

#include "PhysicsEngine.h"
#include "EntityMotionState.h"

class PhysicalEntitySimulation;
using PhysicalEntitySimulationPointer = std::shared_ptr<PhysicalEntitySimulation>;
using SetOfEntityMotionStates = QSet<EntityMotionState*>;

class VectorOfEntityMotionStates: public std::vector<EntityMotionState*> {
public:
    void remove(uint32_t index) {
        assert(index < size());
        if (index < size() - 1) {
            (*this)[index] = back();
        }
        pop_back();
    }
    void removeFirst(EntityMotionState* state) {
        for (uint32_t i = 0; i < size(); ++i) {
            if ((*this)[i] == state) {
                remove(i);
                break;
            }
        }
    }
};

class PhysicalEntitySimulation : public EntitySimulation {
    Q_OBJECT
public:
    PhysicalEntitySimulation();
    ~PhysicalEntitySimulation();

    void init(EntityTreePointer tree, PhysicsEnginePointer engine, EntityEditPacketSender* packetSender);
    void setWorkloadSpace(const workload::SpacePointer space) { _space = space; }

    void addDynamic(EntityDynamicPointer dynamic) override;
    void removeDynamic(const QUuid dynamicID) override;
    void applyDynamicChanges() override;

    void takeDeadAvatarEntities(SetOfEntities& deadEntities);

    virtual void clearEntities() override;
    void queueEraseDomainEntity(const QUuid& id) const override;

signals:
    void entityCollisionWithEntity(const EntityItemID& idA, const EntityItemID& idB, const Collision& collision);

protected: // only called by EntitySimulation
    // overrides for EntitySimulation
    void addEntityToInternalLists(EntityItemPointer entity) override;
    void removeEntityFromInternalLists(EntityItemPointer entity) override;
    void processChangedEntity(const EntityItemPointer& entity) override;
    void processDeadEntities() override;

    void removeOwnershipData(EntityMotionState* motionState);
    void clearOwnershipData();

public:
    virtual void prepareEntityForDelete(EntityItemPointer entity) override;
    void removeDeadEntities();

    void buildPhysicsTransaction(PhysicsEngine::Transaction& transaction);
    void handleProcessedPhysicsTransaction(PhysicsEngine::Transaction& transaction);

    void handleDeactivatedMotionStates(const VectorOfMotionStates& motionStates);
    void handleChangedMotionStates(const VectorOfMotionStates& motionStates);
    void handleCollisionEvents(const CollisionEvents& collisionEvents);

    EntityEditPacketSender* getPacketSender() { return _entityPacketSender; }

    void addOwnershipBid(EntityMotionState* motionState);
    void addOwnership(EntityMotionState* motionState);
    void sendOwnershipBids(uint32_t numSubsteps);
    void sendOwnedUpdates(uint32_t numSubsteps);

private:
    void buildMotionStatesForEntitiesThatNeedThem();

    class ShapeRequest {
    public:
        ShapeRequest() { }
        ShapeRequest(const EntityItemPointer& e) : entity(e) { }
        bool operator<(const ShapeRequest& other) const { return entity.get() < other.entity.get(); }
        bool operator==(const ShapeRequest& other) const { return entity.get() == other.entity.get(); }
        EntityItemPointer entity { nullptr };
        mutable uint64_t shapeHash { 0 };
    };
    SetOfEntities _entitiesToAddToPhysics; // we could also call this: _entitiesThatNeedMotionStates
    SetOfEntities _entitiesToRemoveFromPhysics;
    SetOfEntityMotionStates _incomingChanges; // EntityMotionStates changed by external events
    SetOfMotionStates _physicalObjects; // MotionStates of entities in PhysicsEngine

    using ShapeRequests = std::set<ShapeRequest>;
    ShapeRequests _shapeRequests;

    PhysicsEnginePointer _physicsEngine = nullptr;
    EntityEditPacketSender* _entityPacketSender = nullptr;

    VectorOfEntityMotionStates _owned;
    VectorOfEntityMotionStates _bids;
    SetOfEntities _deadAvatarEntities; // to remove from Avatar's lists
    std::vector<EntityItemPointer> _entitiesToDeleteLater;

    QList<EntityDynamicPointer> _dynamicsToAdd;
    QSet<QUuid> _dynamicsToRemove;
    QRecursiveMutex _dynamicsMutex;

    workload::SpacePointer _space;
    uint64_t _nextBidExpiry;
    uint32_t _lastStepSendPackets { 0 };
    uint32_t _lastWorkDeliveryCount { 0 };
};


typedef std::shared_ptr<PhysicalEntitySimulation> PhysicalEntitySimulationPointer;

#endif // hifi_PhysicalEntitySimulation_h
