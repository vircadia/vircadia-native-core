//
//  EntitySimulation.cpp
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2014.11.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <AACube.h>

#include "EntitySimulation.h"
#include "MovingEntitiesOperator.h"

void EntitySimulation::setEntityTree(EntityTree* tree) {
    if (_entityTree && _entityTree != tree) {
        _mortalEntities.clear();
        _nextExpiry = quint64(-1);
        _updateableEntities.clear();
        _entitiesToBeSorted.clear();
    }
    _entityTree = tree;
}

void EntitySimulation::updateEntities(QSet<EntityItem*>& entitiesToDelete) {
    quint64 now = usecTimestampNow();

    // these methods may accumulate entries in _entitiesToBeDeleted
    expireMortalEntities(now);
    callUpdateOnEntitiesThatNeedIt(now);
    updateEntitiesInternal(now);
    sortEntitiesThatMoved();

    // at this point we harvest _entitiesToBeDeleted
    entitiesToDelete.unite(_entitiesToDelete);
    _entitiesToDelete.clear();
}

void EntitySimulation::expireMortalEntities(const quint64& now) {
    if (now > _nextExpiry) {
        // only search for expired entities if we expect to find one
        _nextExpiry = quint64(-1);
        QSet<EntityItem*>::iterator itemItr = _mortalEntities.begin();
        while (itemItr != _mortalEntities.end()) {
            EntityItem* entity = *itemItr;
            quint64 expiry = entity->getExpiry();
            if (expiry < now) {
                _entitiesToDelete.insert(entity);
                itemItr = _mortalEntities.erase(itemItr);
                _updateableEntities.remove(entity);
                _entitiesToBeSorted.remove(entity);
                removeEntityInternal(entity);
            } else {
                if (expiry < _nextExpiry) {
                    // remeber the smallest _nextExpiry so we know when to start the next search
                    _nextExpiry = expiry;
                }
                ++itemItr;
            }
        }
    }
}

void EntitySimulation::callUpdateOnEntitiesThatNeedIt(const quint64& now) {
    PerformanceTimer perfTimer("updatingEntities");
    QSet<EntityItem*>::iterator itemItr = _updateableEntities.begin();
    while (itemItr != _updateableEntities.end()) {
        EntityItem* entity = *itemItr;
        // TODO: catch transition from needing update to not as a "change" 
        // so we don't have to scan for it here.
        if (!entity->needsToCallUpdate()) {
            itemItr = _updateableEntities.erase(itemItr);
        } else {
            entity->update(now);
            ++itemItr;
        }
    }
}

void EntitySimulation::sortEntitiesThatMoved() {
    // NOTE: this is only for entities that have been moved by THIS EntitySimulation.
    // External changes to entity position/shape are expected to be sorted outside of the EntitySimulation.
    PerformanceTimer perfTimer("sortingEntities");
    MovingEntitiesOperator moveOperator(_entityTree);
    AACube domainBounds(glm::vec3(0.0f,0.0f,0.0f), 1.0f);
    QSet<EntityItem*>::iterator itemItr = _entitiesToBeSorted.begin();
    while (itemItr != _entitiesToBeSorted.end()) {
        EntityItem* entity = *itemItr;
        // check to see if this movement has sent the entity outside of the domain.
        AACube newCube = entity->getMaximumAACube();
        if (!domainBounds.touches(newCube)) {
            qDebug() << "Entity " << entity->getEntityItemID() << " moved out of domain bounds.";
            _entitiesToDelete.insert(entity);
            _mortalEntities.remove(entity);
            _updateableEntities.remove(entity);
            removeEntityInternal(entity);
        } else {
            moveOperator.addEntityToMoveList(entity, newCube);
        }
        ++itemItr;
    }
    _entitiesToBeSorted.clear();

    if (moveOperator.hasMovingEntities()) {
        PerformanceTimer perfTimer("recurseTreeWithOperator");
        _entityTree->recurseTreeWithOperator(&moveOperator);
        moveOperator.finish();
    }
}

void EntitySimulation::addEntity(EntityItem* entity) {
    assert(entity);
    if (entity->isMortal()) {
        _mortalEntities.insert(entity);
        quint64 expiry = entity->getExpiry();
        if (expiry < _nextExpiry) {
            _nextExpiry = expiry;
        }
    }
    if (entity->needsToCallUpdate()) {
        _updateableEntities.insert(entity);
    }
    addEntityInternal(entity);
}

void EntitySimulation::removeEntity(EntityItem* entity) {
    assert(entity);
    _updateableEntities.remove(entity);
    _mortalEntities.remove(entity);
    _entitiesToBeSorted.remove(entity);
    _entitiesToDelete.remove(entity);
    removeEntityInternal(entity);
}

void EntitySimulation::entityChanged(EntityItem* entity) {
    assert(entity);

    // Although it is not the responsibility of the EntitySimulation to sort the tree for EXTERNAL changes
    // it IS responsibile for triggering deletes for entities that leave the bounds of the domain, hence 
    // we must check for that case here, however we rely on the change event to have set DIRTY_POSITION flag.
    bool wasRemoved = false;
    uint32_t dirtyFlags = entity->getDirtyFlags();
    if (dirtyFlags & EntityItem::DIRTY_POSITION) {
        AACube domainBounds(glm::vec3(0.0f,0.0f,0.0f), 1.0f);
        AACube newCube = entity->getMaximumAACube();
        if (!domainBounds.touches(newCube)) {
            qDebug() << "Entity " << entity->getEntityItemID() << " moved out of domain bounds.";
            _entitiesToDelete.insert(entity);
            _mortalEntities.remove(entity);
            _updateableEntities.remove(entity);
            removeEntityInternal(entity);
            wasRemoved = true;
        }
    }
    if (!wasRemoved) {
        if (dirtyFlags & EntityItem::DIRTY_LIFETIME) {
            if (entity->isMortal()) {
                _mortalEntities.insert(entity);
                quint64 expiry = entity->getExpiry();
                if (expiry < _nextExpiry) {
                    _nextExpiry = expiry;
                }
            } else {
                _mortalEntities.remove(entity);
            }
            entity->clearDirtyFlags(EntityItem::DIRTY_LIFETIME);
        }
        if (entity->needsToCallUpdate()) {
            _updateableEntities.insert(entity);
        } else {
            _updateableEntities.remove(entity);
        }
        entityChangedInternal(entity);
    }
    entity->clearDirtyFlags();
}

void EntitySimulation::clearEntities() {
    _mortalEntities.clear();
    _nextExpiry = quint64(-1);
    _updateableEntities.clear();
    _entitiesToBeSorted.clear();
    clearEntitiesInternal();
}
