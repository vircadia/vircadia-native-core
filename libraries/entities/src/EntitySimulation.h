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

#include "EntityTree.h"

class EntitySimulation {
public:
    EntitySimulation() : _entityTree(NULL) { }
    virtual ~EntitySimulation() {}

    /// \param tree pointer to EntityTree which is stored internally
    virtual void setEntityTree(EntityTree* tree);

    /// \param[out] entitiesToDelete list of entities removed from simulation and should be deleted.
    virtual void updateEntities(QSet<EntityItem*>& entitiesToDelete) = 0;

    /// \param entity pointer to EntityItem to add to the simulation
    /// \sideeffect the EntityItem::_simulationState member may be updated to indicate membership to internal list
    virtual void addEntity(EntityItem* entity) = 0;

    /// \param entity pointer to EntityItem to removed from the simulation
    /// \sideeffect the EntityItem::_simulationState member may be updated to indicate non-membership to internal list
    virtual void removeEntity(EntityItem* entity) = 0;

    /// \param entity pointer to EntityItem to that may have changed in a way that would affect its simulation
    virtual void entityChanged(EntityItem* entity) = 0;

    virtual void clearEntities() = 0;

    EntityTree* getEntityTree() { return _entityTree; }

protected:
    EntityTree* _entityTree;
};

#endif // hifi_EntitySimulation_h
