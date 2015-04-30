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
#include "EntitiesLogging.h"
#include "MovingEntitiesOperator.h"

void EntitySimulation::setEntityTree(EntityTree* tree) {
    if (_entityTree && _entityTree != tree) {
        _mortalEntities.clear();
        _nextExpiry = quint64(-1);
        _entitiesToUpdate.clear();
        _entitiesToSort.clear();
    }
    _entityTree = tree;
}

void EntitySimulation::updateEntities() {
    quint64 now = usecTimestampNow();

    // these methods may accumulate entries in _entitiesToBeDeleted
    expireMortalEntities(now);
    callUpdateOnEntitiesThatNeedIt(now);
    updateEntitiesInternal(now);
    sortEntitiesThatMoved();
}

void EntitySimulation::getEntitiesToDelete(SetOfEntities& entitiesToDelete) {
    SetOfEntities::iterator entityItr = _entitiesToDelete.begin();
    while (entityItr != _entitiesToDelete.end()) {
        EntityItem* entity = *entityItr;
        if (entity->_simulation != this) {
            if (entity->_element) {
                // this entity is still in its tree, so we insert into the external list
                entitiesToDelete.insert(entity);
            } else {
                // no more backpointers, so it should be safe to delete
                delete entity;
            }
            // we're done with the entity in this context, so remove it from our internal list
            entityItr = _entitiesToDelete.erase(entityItr);
        } else {
            // internal cleanup will happen later (probably in updateEntitiesInternal())
            ++entityItr;
        }
    }
}

// protected
void EntitySimulation::expireMortalEntities(const quint64& now) {
    if (now > _nextExpiry) {
        // only search for expired entities if we expect to find one
        _nextExpiry = quint64(-1);
        SetOfEntities::iterator itemItr = _mortalEntities.begin();
        while (itemItr != _mortalEntities.end()) {
            EntityItem* entity = *itemItr;
            quint64 expiry = entity->getExpiry();
            if (expiry < now) {
                _entitiesToDelete.insert(entity);
                itemItr = _mortalEntities.erase(itemItr);
                _entitiesToUpdate.remove(entity);
                _entitiesToSort.remove(entity);
                deleteEntityInternal(entity);
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

// protected
void EntitySimulation::callUpdateOnEntitiesThatNeedIt(const quint64& now) {
    PerformanceTimer perfTimer("updatingEntities");
    SetOfEntities::iterator itemItr = _entitiesToUpdate.begin();
    while (itemItr != _entitiesToUpdate.end()) {
        EntityItem* entity = *itemItr;
        // TODO: catch transition from needing update to not as a "change" 
        // so we don't have to scan for it here.
        if (!entity->needsToCallUpdate()) {
            itemItr = _entitiesToUpdate.erase(itemItr);
        } else {
            entity->update(now);
            ++itemItr;
        }
    }
}

// protected
void EntitySimulation::sortEntitiesThatMoved() {
    // NOTE: this is only for entities that have been moved by THIS EntitySimulation.
    // External changes to entity position/shape are expected to be sorted outside of the EntitySimulation.
    PerformanceTimer perfTimer("sortingEntities");
    MovingEntitiesOperator moveOperator(_entityTree);
    AACube domainBounds(glm::vec3(0.0f,0.0f,0.0f), (float)TREE_SCALE);
    SetOfEntities::iterator itemItr = _entitiesToSort.begin();
    while (itemItr != _entitiesToSort.end()) {
        EntityItem* entity = *itemItr;
        // check to see if this movement has sent the entity outside of the domain.
        AACube newCube = entity->getMaximumAACube();
        if (!domainBounds.touches(newCube)) {
            qCDebug(entities) << "Entity " << entity->getEntityItemID() << " moved out of domain bounds.";
            _entitiesToDelete.insert(entity);
            _mortalEntities.remove(entity);
            _entitiesToUpdate.remove(entity);
            deleteEntityInternal(entity);
            itemItr = _entitiesToSort.erase(itemItr);
        } else {
            moveOperator.addEntityToMoveList(entity, newCube);
            ++itemItr;
        }
    }
    if (moveOperator.hasMovingEntities()) {
        PerformanceTimer perfTimer("recurseTreeWithOperator");
        _entityTree->recurseTreeWithOperator(&moveOperator);
    }

    sortEntitiesThatMovedInternal();
    _entitiesToSort.clear();
}

void EntitySimulation::addEntity(EntityItem* entity) {
    assert(entity);
    if (!entity->_simulation) {
        entity->_simulation = this;
        if (entity->isMortal()) {
            _mortalEntities.insert(entity);
            quint64 expiry = entity->getExpiry();
            if (expiry < _nextExpiry) {
                _nextExpiry = expiry;
            }
        }
        if (entity->needsToCallUpdate()) {
            _entitiesToUpdate.insert(entity);
        }
        addEntityInternal(entity);
    
        // DirtyFlags are used to signal changes to entities that have already been added, 
        // so we can clear them for this entity which has just been added.
        entity->clearDirtyFlags();
    }
}

void EntitySimulation::removeEntity(EntityItem* entity) {
    assert(entity);
    if (entity->_simulation == this) {
        _entitiesToUpdate.remove(entity);
        _mortalEntities.remove(entity);
        _entitiesToSort.remove(entity);
        if (entity->_element) {
            // some EntityTreeElement still references this entity, but it's being removed from this simulation
            _entitiesToDelete.remove(entity);
            removeEntityInternal(entity);
        } else {
            // we're the last to reference this entity, so we really need to delete it
            deleteEntityInternal(entity);
        }
    } else if (!entity->_element) {
        // nothing else is referencing this entity, so we delete it now
        delete entity;
    }
}

void EntitySimulation::deleteEntity(EntityItem* entity) {
    assert(entity);
    if (entity->_simulation == this) {
        _entitiesToUpdate.remove(entity);
        _mortalEntities.remove(entity);
        _entitiesToSort.remove(entity);
        deleteEntityInternal(entity);
    } else {
        if (entity->_element) {
            // some EntityTreeElement still references this entity, so we put it on the list 
            // which will be harvested by the tree later
            _entitiesToDelete.insert(entity);
        } else {
            // nothing else is referencing this entity, so we delete it now
            delete entity;
        }
    }
}

void EntitySimulation::changeEntity(EntityItem* entity) {
    assert(entity);
    if (entity->_simulation == this) {
        // Although it is not the responsibility of the EntitySimulation to sort the tree for EXTERNAL changes
        // it IS responsibile for triggering deletes for entities that leave the bounds of the domain, hence 
        // we must check for that case here, however we rely on the change event to have set DIRTY_POSITION flag.
        bool wasRemoved = false;
        uint32_t dirtyFlags = entity->getDirtyFlags();
        if (dirtyFlags & EntityItem::DIRTY_POSITION) {
            AACube domainBounds(glm::vec3(0.0f,0.0f,0.0f), (float)TREE_SCALE);
            AACube newCube = entity->getMaximumAACube();
            if (!domainBounds.touches(newCube)) {
                qCDebug(entities) << "Entity " << entity->getEntityItemID() << " moved out of domain bounds.";
                _entitiesToDelete.insert(entity);
                _mortalEntities.remove(entity);
                _entitiesToUpdate.remove(entity);
                deleteEntityInternal(entity);
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
                _entitiesToUpdate.insert(entity);
            } else {
                _entitiesToUpdate.remove(entity);
            }
            changeEntityInternal(entity);
        }
    } else {
        // this entity is not yet in this simulation but something (the tree) assumes that it is --> try to add it
        addEntity(entity);
    }
}

void EntitySimulation::clearEntities() {
    _mortalEntities.clear();
    _nextExpiry = quint64(-1);
    _entitiesToUpdate.clear();
    _entitiesToSort.clear();
    clearEntitiesInternal();
}

