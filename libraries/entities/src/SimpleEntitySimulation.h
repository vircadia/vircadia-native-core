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

/// provides simple velocity + gravity extrapolation of EntityItem's

class SimpleEntitySimulation : public EntitySimulation {
public:
    SimpleEntitySimulation() : EntitySimulation() { }
    virtual ~SimpleEntitySimulation() { setEntityTree(NULL); }

    virtual void updateEntities(QSet<EntityItem*>& entitiesToDelete);

    virtual void addEntity(EntityItem* entity);
    virtual void removeEntity(EntityItem* entity);
    virtual void entityChanged(EntityItem* entity);

    virtual void clearEntities();

protected:
    void updateEntityState(EntityItem* entity);
    void clearEntityState(EntityItem* entity);

    QList<EntityItem*>& getMovingEntities() { return _movingEntities; }

    void updateChangedEntities(quint64 now, QSet<EntityItem*>& entitiesToDelete);
    void updateMovingEntities(quint64 now, QSet<EntityItem*>& entitiesToDelete);
    void updateMortalEntities(quint64 now, QSet<EntityItem*>& entitiesToDelete);

    QSet<EntityItem*> _changedEntities; // entities that have changed in the last frame
    QList<EntityItem*> _movingEntities; // entities that need to be updated
    QList<EntityItem*> _mortalEntities; // non-moving entities that need to be checked for expiry
};

#endif // hifi_SimpleEntitySimulation_h
