//
//  DeleteEntityOperator.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 8/11/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DeleteEntityOperator.h"

#include "EntityItem.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "EntitiesLogging.h"

DeleteEntityOperator::DeleteEntityOperator(EntityTreePointer tree, const EntityItemID& searchEntityID) :
    _tree(tree),
    _changeTime(usecTimestampNow()),
    _foundCount(0),
    _lookingCount(0)
{
    addEntityIDToDeleteList(searchEntityID);
}

DeleteEntityOperator::~DeleteEntityOperator() {
}

DeleteEntityOperator::DeleteEntityOperator(EntityTreePointer tree) :
    _tree(tree),
    _changeTime(usecTimestampNow()),
    _foundCount(0),
    _lookingCount(0)
{
}

void DeleteEntityOperator::addEntityIDToDeleteList(const EntityItemID& searchEntityID) {
    // check our tree, to determine if this entity is known
    EntityToDeleteDetails details;
    details.containingElement = _tree->getContainingElement(searchEntityID);
    if (details.containingElement) {
        details.entity = details.containingElement->getEntityWithEntityItemID(searchEntityID);
        if (!details.entity) {
            //assert(false);
            qCDebug(entities) << "that's UNEXPECTED, we got a _containingElement, but couldn't find the oldEntity!";
        } else {
            details.cube = details.containingElement->getAACube();
            _entitiesToDelete << details;
            _lookingCount++;
        }
    }
}

void DeleteEntityOperator::addEntityToDeleteList(const EntityItemPointer& entity) {
    assert(entity && entity->getElement());
    EntityToDeleteDetails details;
    details.entity = entity;
    details.containingElement = entity->getElement();
    details.cube = details.containingElement->getAACube();
    _entitiesToDelete << details;
    _lookingCount++;
}

// does this entity tree element contain the old entity
bool DeleteEntityOperator::subTreeContainsSomeEntitiesToDelete(const OctreeElementPointer& element) {
    bool containsEntity = false;

    // If we don't have an old entity, then we don't contain the entity, otherwise
    // check the bounds
    if (_entitiesToDelete.size() > 0) {
        const AACube& elementCube = element->getAACube();
        foreach(const EntityToDeleteDetails& details, _entitiesToDelete) {
            if (elementCube.contains(details.cube)) {
                containsEntity = true;
                break; // if it contains at least one, we're good to go
            }
        }
    }
    return containsEntity;
}

bool DeleteEntityOperator::preRecursion(const OctreeElementPointer& element) {
    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);

    // In Pre-recursion, we're generally deciding whether or not we want to recurse this
    // path of the tree. For this operation, we want to recurse the branch of the tree if:
    //   * We have not yet found the all entities, and 
    //   * this branch contains our some of the entities we're looking for.
    
    bool keepSearching = false; // assume we don't need to search any more
    
    // If we haven't yet found all the entities, and this sub tree contains at least one of our
    // entities, then we need to keep searching.
    if ((_foundCount < _lookingCount) && subTreeContainsSomeEntitiesToDelete(element)) {

        // check against each of our search entities
        foreach(const EntityToDeleteDetails& details, _entitiesToDelete) {

            // If this is the element we're looking for, then ask it to remove the old entity
            // and we can stop searching.
            if (entityTreeElement == details.containingElement) {
                EntityItemPointer theEntity = details.entity;
                bool entityDeleted = entityTreeElement->removeEntityItem(theEntity, true); // remove it from the element
                assert(entityDeleted);
                (void)entityDeleted; // quiet warning about unused variable
                _tree->clearEntityMapEntry(details.entity->getEntityItemID());
                _foundCount++;
            }
        }
        
        // if we haven't found all of our search for entities, then keep looking
        keepSearching = (_foundCount < _lookingCount);
    }

    return keepSearching; // if we haven't yet found it, keep looking
}

bool DeleteEntityOperator::postRecursion(const OctreeElementPointer& element) {
    // Post-recursion is the unwinding process. For this operation, while we
    // unwind we want to mark the path as being dirty if we changed it below.
    // We might have two paths, one for the old entity and one for the new entity.
    bool keepSearching = (_foundCount < _lookingCount);

    // As we unwind, if we're in either of these two paths, we mark our element
    // as dirty.
    if ((subTreeContainsSomeEntitiesToDelete(element))) {
        element->markWithChangedTime();
    }
    
    // It should always be ok to prune children. Because we are only in this PostRecursion function if
    // we've already finished processing all of the children of this current element. If any of those
    // children are the containing element for any entity in our lists of entities to delete, then they
    // must have already deleted the entity, and they are safe to prune. Since this operation doesn't
    // ever add any elements we don't have to worry about memory being reused within this recursion pass.
    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
    entityTreeElement->pruneChildren(); // take this opportunity to prune any empty leaves
    return keepSearching; // if we haven't yet found it, keep looking
}

