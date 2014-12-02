//
//  SimpleEntitySimulation.cpp
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2014.11.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <AACube.h>
#include <PerfStat.h>

#include "EntityItem.h"
#include "MovingEntitiesOperator.h"
#include "SimpleEntitySimulation.h"

void SimpleEntitySimulation::updateEntities(QSet<EntityItem*>& entitiesToDelete) {
    quint64 now = usecTimestampNow();
    updateChangedEntities(now, entitiesToDelete);
    updateMovingEntities(now, entitiesToDelete);
    updateMortalEntities(now, entitiesToDelete);
}

void SimpleEntitySimulation::addEntity(EntityItem* entity) {
    assert(entity && entity->getSimulationState() == EntityItem::Static);
    EntityItem::SimulationState state = entity->computeSimulationState();
    switch(state) {
        case EntityItem::Moving:
            _movingEntities.push_back(entity);
            entity->setSimulationState(state);
            break;
        case EntityItem::Mortal:
            _mortalEntities.push_back(entity);
            entity->setSimulationState(state);
            break;
        case EntityItem::Static:
        default:
            break;
    }
}

void SimpleEntitySimulation::removeEntity(EntityItem* entity) {
    assert(entity);
    // make sure to remove it from any of our simulation lists
    EntityItem::SimulationState state = entity->getSimulationState();
    switch (state) {
        case EntityItem::Moving:
            _movingEntities.removeAll(entity);
            break;
        case EntityItem::Mortal:
            _mortalEntities.removeAll(entity);
            break;

        default:
            break;
    }
    entity->setSimulationState(EntityItem::Static);
    _changedEntities.remove(entity);
}

void SimpleEntitySimulation::entityChanged(EntityItem* entity) {
    assert(entity);
    // we batch all changes and deal with them in updateChangedEntities()
    _changedEntities.insert(entity);
}

void SimpleEntitySimulation::clearEntities() {
    foreach (EntityItem* entity, _changedEntities) {
        entity->clearUpdateFlags();
        entity->setSimulationState(EntityItem::Static);
    }
    _changedEntities.clear();
    _movingEntities.clear();
    _mortalEntities.clear();
}
   
void SimpleEntitySimulation::updateChangedEntities(quint64 now, QSet<EntityItem*>& entitiesToDelete) {
    foreach (EntityItem* entity, _changedEntities) {
        // check to see if the lifetime has expired, for immortal entities this is always false
        if (entity->lifetimeHasExpired()) {
            qDebug() << "Lifetime has expired for entity:" << entity->getEntityItemID();
            entitiesToDelete.insert(entity);
            clearEntityState(entity);
        } else {
            updateEntityState(entity);
        }
        entity->clearUpdateFlags();
    }
    _changedEntities.clear();
}

void SimpleEntitySimulation::updateMovingEntities(quint64 now, QSet<EntityItem*>& entitiesToDelete) {
    if (_entityTree && _movingEntities.size() > 0) {
        PerformanceTimer perfTimer("_movingEntities");
        MovingEntitiesOperator moveOperator(_entityTree);
        QList<EntityItem*>::iterator item_itr = _movingEntities.begin();
        while (item_itr != _movingEntities.end()) {
            EntityItem* entity = *item_itr;

            // always check to see if the lifetime has expired, for immortal entities this is always false
            if (entity->lifetimeHasExpired()) {
                qDebug() << "Lifetime has expired for entity:" << entity->getEntityItemID();
                entitiesToDelete.insert(entity);
                // remove entity from the list
                item_itr = _movingEntities.erase(item_itr);
                entity->setSimulationState(EntityItem::Static);
            } else {
                AACube oldCube = entity->getMaximumAACube();
                entity->update(now);
                AACube newCube = entity->getMaximumAACube();
                
                // check to see if this movement has sent the entity outside of the domain.
                AACube domainBounds(glm::vec3(0.0f,0.0f,0.0f), 1.0f);
                if (!domainBounds.touches(newCube)) {
                    qDebug() << "Entity " << entity->getEntityItemID() << " moved out of domain bounds.";
                    entitiesToDelete.insert(entity);
                    // remove entity from the list
                    item_itr = _movingEntities.erase(item_itr);
                    entity->setSimulationState(EntityItem::Static);
                } else {
                    moveOperator.addEntityToMoveList(entity, oldCube, newCube);
                    EntityItem::SimulationState newState = entity->computeSimulationState();
                    if (newState != EntityItem::Moving) {
                        if (newState == EntityItem::Mortal) {
                            _mortalEntities.push_back(entity);
                        }
                        // remove entity from the list
                        item_itr = _movingEntities.erase(item_itr);
                        entity->setSimulationState(newState);
                    } else {
                        ++item_itr;
                    }
                }
            }
        }
        if (moveOperator.hasMovingEntities()) {
            PerformanceTimer perfTimer("recurseTreeWithOperator");
            _entityTree->recurseTreeWithOperator(&moveOperator);
        }
    }
}

void SimpleEntitySimulation::updateMortalEntities(quint64 now, QSet<EntityItem*>& entitiesToDelete) {
    QList<EntityItem*>::iterator item_itr = _mortalEntities.begin();
    while (item_itr != _mortalEntities.end()) {
        EntityItem* entity = *item_itr;
        // always check to see if the lifetime has expired, for immortal entities this is always false
        if (entity->lifetimeHasExpired()) {
            qDebug() << "Lifetime has expired for entity:" << entity->getEntityItemID();
            entitiesToDelete.insert(entity);
            // remove entity from the list
            item_itr = _mortalEntities.erase(item_itr);
            entity->setSimulationState(EntityItem::Static);
        } else {
            EntityItem::SimulationState newState = entity->computeSimulationState();
            if (newState != EntityItem::Mortal) {
                // check to see if this entity is moving
                if (newState == EntityItem::Moving) {
                    entity->update(now);
                    _movingEntities.push_back(entity);
                }
                // remove entity from the list
                item_itr = _mortalEntities.erase(item_itr);
                entity->setSimulationState(newState);
            } else {
                ++item_itr;
            }
        }
    }
}

void SimpleEntitySimulation::updateEntityState(EntityItem* entity) {
    EntityItem::SimulationState oldState = entity->getSimulationState();
    EntityItem::SimulationState newState = entity->computeSimulationState();
    if (newState != oldState) {
        switch (oldState) {
            case EntityItem::Moving:
                _movingEntities.removeAll(entity);
                break;
    
            case EntityItem::Mortal:
                _mortalEntities.removeAll(entity);
                break;
    
            default:
                break;
        }
    
        switch (newState) {
            case EntityItem::Moving:
                _movingEntities.push_back(entity);
                break;
    
            case EntityItem::Mortal:
                _mortalEntities.push_back(entity);
                break;
    
            default:
                break;
        }
        entity->setSimulationState(newState);
    }
}

void SimpleEntitySimulation::clearEntityState(EntityItem* entity) {
    EntityItem::SimulationState oldState = entity->getSimulationState();
    switch (oldState) {
        case EntityItem::Moving:
            _movingEntities.removeAll(entity);
            break;

        case EntityItem::Mortal:
            _mortalEntities.removeAll(entity);
            break;

        default:
            break;
    }
    entity->setSimulationState(EntityItem::Static);
}


