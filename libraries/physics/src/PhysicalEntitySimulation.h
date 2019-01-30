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

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

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

    virtual void addDynamic(EntityDynamicPointer dynamic) override;
    virtual void applyDynamicChanges() override;

    virtual void takeDeadEntities(SetOfEntities& deadEntities) override;
    void takeDeadAvatarEntities(SetOfEntities& deadEntities);

signals:
    void entityCollisionWithEntity(const EntityItemID& idA, const EntityItemID& idB, const Collision& collision);

protected: // only called by EntitySimulation
    // overrides for EntitySimulation
    virtual void updateEntitiesInternal(uint64_t now) override;
    virtual void addEntityInternal(EntityItemPointer entity) override;
    virtual void removeEntityInternal(EntityItemPointer entity) override;
    virtual void changeEntityInternal(EntityItemPointer entity) override;
    virtual void clearEntitiesInternal() override;

    void removeOwnershipData(EntityMotionState* motionState);
    void clearOwnershipData();

public:
    virtual void prepareEntityForDelete(EntityItemPointer entity) override;

    const VectorOfMotionStates& getObjectsToRemoveFromPhysics();
    void deleteObjectsRemovedFromPhysics();

    void getObjectsToAddToPhysics(VectorOfMotionStates& result);
    void setObjectsToChange(const VectorOfMotionStates& objectsToChange);
    void getObjectsToChange(VectorOfMotionStates& result);

    void handleDeactivatedMotionStates(const VectorOfMotionStates& motionStates);
    void handleChangedMotionStates(const VectorOfMotionStates& motionStates);
    void handleCollisionEvents(const CollisionEvents& collisionEvents);

    EntityEditPacketSender* getPacketSender() { return _entityPacketSender; }

    void addOwnershipBid(EntityMotionState* motionState);
    void addOwnership(EntityMotionState* motionState);
    void sendOwnershipBids(uint32_t numSubsteps);
    void sendOwnedUpdates(uint32_t numSubsteps);

private:
    SetOfEntities _entitiesToAddToPhysics;
    SetOfEntities _entitiesToRemoveFromPhysics;

    VectorOfMotionStates _objectsToDelete;

    SetOfEntityMotionStates _incomingChanges; // EntityMotionStates that have changed from external sources
                                              // and need their RigidBodies updated

    SetOfMotionStates _physicalObjects; // MotionStates of entities in PhysicsEngine

    PhysicsEnginePointer _physicsEngine = nullptr;
    EntityEditPacketSender* _entityPacketSender = nullptr;

    VectorOfEntityMotionStates _owned;
    VectorOfEntityMotionStates _bids;
    SetOfEntities _deadAvatarEntities;
    workload::SpacePointer _space;
    uint64_t _nextBidExpiry;
    uint32_t _lastStepSendPackets { 0 };
};


typedef std::shared_ptr<PhysicalEntitySimulation> PhysicalEntitySimulationPointer;

#endif // hifi_PhysicalEntitySimulation_h
