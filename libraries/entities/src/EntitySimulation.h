//
//  EntitySimulation.h
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2014.11.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntitySimulation_h
#define hifi_EntitySimulation_h

#include <QSet>

#include "EntityItemID.h"
#include "EntityTree.h"

class EntitySimulation {
public:
    EntitySimulation(EntityTree* tree) : _myTree(tree) { assert(tree); }
    virtual ~EntitySimulation() { _myTree = NULL; }

    /// \sideeffect For each EntityItem* that EntitySimulation puts on entitiesToDelete it will automatically 
    /// removeEntity() on any internal lists -- no need to call removeEntity() for that one later.
    virtual void update(QSet<EntityItemID>& entitiesToDelete) = 0;

    virtual void addEntity(EntityItem* entity) = 0;
    virtual void removeEntity(EntityItem* entity) = 0;
    virtual void updateEntity(EntityItem* entity) = 0;

protected:
    EntityTree* _myTree;
};

#endif // hifi_EntitySimulation_h
