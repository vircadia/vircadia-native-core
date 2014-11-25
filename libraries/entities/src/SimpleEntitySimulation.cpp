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

void SimpleEntitySimulation::update(QSet<EntityItemID>& entitiesToDelete) {
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
    _changedEntities.remove(entity);
}

void SimpleEntitySimulation::updateEntity(EntityItem* entity) {
    assert(entity);
    // we'll deal with thsi change later
    _changedEntities.insert(entity);
}
   
void SimpleEntitySimulation::updateChangedEntities(quint64 now, QSet<EntityItemID>& entitiesToDelete) {
    foreach (EntityItem* entity, _changedEntities) {
        // check to see if the lifetime has expired, for immortal entities this is always false
        if (entity->lifetimeHasExpired()) {
            qDebug() << "Lifetime has expired for entity:" << entity->getEntityItemID();
            entitiesToDelete << entity->getEntityItemID();
            clearEntityState(entity);
        } else {
            updateEntityState(entity);
        }
        entity->clearUpdateFlags();
    }
    _changedEntities.clear();
}

void SimpleEntitySimulation::updateMovingEntities(quint64 now, QSet<EntityItemID>& entitiesToDelete) {
    if (_movingEntities.size() > 0) {
        PerformanceTimer perfTimer("_movingEntities");
        MovingEntitiesOperator moveOperator(_myTree);
        QList<EntityItem*>::iterator item_itr = _movingEntities.begin();
        while (item_itr != _movingEntities.end()) {
            EntityItem* thisEntity = *item_itr;

            // always check to see if the lifetime has expired, for immortal entities this is always false
            if (thisEntity->lifetimeHasExpired()) {
                qDebug() << "Lifetime has expired for entity:" << thisEntity->getEntityItemID();
                entitiesToDelete << thisEntity->getEntityItemID();
                // remove thisEntity from the list
                item_itr = _movingEntities.erase(item_itr);
                thisEntity->setSimulationState(EntityItem::Static);
            } else {
                AACube oldCube = thisEntity->getMaximumAACube();
                thisEntity->update(now);
                AACube newCube = thisEntity->getMaximumAACube();
                
                // check to see if this movement has sent the entity outside of the domain.
                AACube domainBounds(glm::vec3(0.0f,0.0f,0.0f), 1.0f);
                if (!domainBounds.touches(newCube)) {
                    qDebug() << "Entity " << thisEntity->getEntityItemID() << " moved out of domain bounds.";
                    entitiesToDelete << thisEntity->getEntityItemID();
                    // remove thisEntity from the list
                    item_itr = _movingEntities.erase(item_itr);
                    thisEntity->setSimulationState(EntityItem::Static);
                } else {
                    moveOperator.addEntityToMoveList(thisEntity, oldCube, newCube);
                    EntityItem::SimulationState newState = thisEntity->computeSimulationState();
                    if (newState != EntityItem::Moving) {
                        if (newState == EntityItem::Mortal) {
                            _mortalEntities.push_back(thisEntity);
                        }
                        // remove thisEntity from the list
                        item_itr = _movingEntities.erase(item_itr);
                        thisEntity->setSimulationState(newState);
                    } else {
                        ++item_itr;
                    }
                }
            }
        }
        if (moveOperator.hasMovingEntities()) {
            PerformanceTimer perfTimer("recurseTreeWithOperator");
            _myTree->recurseTreeWithOperator(&moveOperator);
        }
    }
}

void SimpleEntitySimulation::updateMortalEntities(quint64 now, QSet<EntityItemID>& entitiesToDelete) {
    QList<EntityItem*>::iterator item_itr = _mortalEntities.begin();
    while (item_itr != _mortalEntities.end()) {
        EntityItem* thisEntity = *item_itr;
        thisEntity->update(now);
        // always check to see if the lifetime has expired, for immortal entities this is always false
        if (thisEntity->lifetimeHasExpired()) {
            qDebug() << "Lifetime has expired for entity:" << thisEntity->getEntityItemID();
            entitiesToDelete << thisEntity->getEntityItemID();
            // remove thisEntity from the list
            item_itr = _mortalEntities.erase(item_itr);
            thisEntity->setSimulationState(EntityItem::Static);
        } else {
            // check to see if this entity is no longer moving
            EntityItem::SimulationState newState = thisEntity->computeSimulationState();
            if (newState != EntityItem::Mortal) {
                if (newState == EntityItem::Moving) {
                    _movingEntities.push_back(thisEntity);
                }
                // remove thisEntity from the list
                item_itr = _mortalEntities.erase(item_itr);
                thisEntity->setSimulationState(newState);
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


