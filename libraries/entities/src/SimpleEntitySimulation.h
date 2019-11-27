//
//  SimpleEntitySimulation.h
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2014.11.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SimpleEntitySimulation_h
#define hifi_SimpleEntitySimulation_h

#include "EntitySimulation.h"

class SimpleEntitySimulation;
using SimpleEntitySimulationPointer = std::shared_ptr<SimpleEntitySimulation>;


/// provides simple velocity + gravity extrapolation of EntityItem's

class SimpleEntitySimulation : public EntitySimulation {
public:
    SimpleEntitySimulation() : EntitySimulation() { }
    ~SimpleEntitySimulation() { clearEntities(); }

    void clearOwnership(const QUuid& ownerID);
    void clearEntities() override;
    void updateEntities() override;

protected:
    void addEntityToInternalLists(EntityItemPointer entity) override;
    void removeEntityFromInternalLists(EntityItemPointer entity) override;
    void processChangedEntity(const EntityItemPointer& entity) override;

    void sortEntitiesThatMoved() override;

    void expireStaleOwnerships(uint64_t now);
    void stopOwnerlessEntities(uint64_t now);

    SetOfEntities _entitiesWithSimulationOwner;
    SetOfEntities _entitiesThatNeedSimulationOwner;
    uint64_t _nextOwnerlessExpiry { 0 };
    uint64_t _nextStaleOwnershipExpiry { (uint64_t)(-1) };
};

#endif // hifi_SimpleEntitySimulation_h
