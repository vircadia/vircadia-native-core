//
//  PhysicalEntitySimulation.h
//  libraries/physcis/src
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

#include <QSet>
#include <QVector>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <EntityItem.h>
#include <EntitySimulation.h>

#include "PhysicsEngine.h"

typedef QSet<ObjectMotionState*> SetOfMotionStates;
typedef QVector<ObjectMotionState*> VectorOfMotionStates;

class PhysicalEntitySimulation :public EntitySimulation {
public:
    PhysicalEntitySimulation();
    ~PhysicalEntitySimulation();

    void init(EntityTree* tree, PhysicsEngine* engine, EntityEditPacketSender* packetSender);

    // overrides for EntitySimulation
    void updateEntitiesInternal(const quint64& now);
    void addEntityInternal(EntityItem* entity);
    void removeEntityInternal(EntityItem* entity);
    void deleteEntityInternal(EntityItem* entity);
    void entityChangedInternal(EntityItem* entity);
    void sortEntitiesThatMovedInternal();
    void clearEntitiesInternal();

    VectorOfMotionStates& getObjectsToRemove();
    VectorOfMotionStates& getObjectsToAdd();
    VectorOfMotionStates& getObjectsToChange();

    void handleOutgoingChanges(SetOfMotionStates& motionStates);

private:
    void bump(EntityItem* bumpEntity);

    // incoming changes
    SetOfEntities _pendingRemoves; // entities to be removed from simulation
    SetOfEntities _pendingAdds; // entities to be be added to simulation
    SetOfEntities _pendingChanges; // entities already in simulation that need to be changed

    // outgoing changes
    SetOfEntities _outgoingChanges; // entities for which we need to send updates to entity-server

    SetOfMotionStates _physicalEntities; // MotionStates of entities in PhysicsEngine
    VectorOfMotionStates _tempSet; // temporary list valid immediately after call to getObjectsToRemove/Add/Update()

    ShapeManager* _shapeManager = nullptr;
    PhysicsEngine* _physicsEngine = nullptr;
    EntityEditPacketSender* _entityPacketSender = nullptr;

    uint32_t _lastNumSubstepsAtUpdateInternal = 0;
};

#endif // hifi_PhysicalEntitySimulation_h
