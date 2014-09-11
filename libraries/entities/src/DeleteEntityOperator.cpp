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

#include "EntityItem.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"

#include "DeleteEntityOperator.h"

DeleteEntityOperator::DeleteEntityOperator(EntityTree* tree, const EntityItemID& searchEntityID) :
    _tree(tree),
    _changeTime(usecTimestampNow()),
    _foundCount(0),
    _lookingCount(0)
{
    addEntityIDToDeleteList(searchEntityID);
}

DeleteEntityOperator::~DeleteEntityOperator() {
}

DeleteEntityOperator::DeleteEntityOperator(EntityTree* tree) :
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
            qDebug() << "that's UNEXPECTED, we got a _containingElement, but couldn't find the oldEntity!";
        } else {
            details.cube = details.containingElement->getAACube();
            _entitiesToDelete << details;
            _lookingCount++;
            _tree->trackDeletedEntity(searchEntityID);
            // before deleting any entity make sure to remove it from our Mortal, Changing, and Moving lists
            _tree->removeEntityFromSimulationLists(searchEntityID);
        }
    }
}


// does this entity tree element contain the old entity
bool DeleteEntityOperator::subTreeContainsSomeEntitiesToDelete(OctreeElement* element) {
    bool containsEntity = false;

    // If we don't have an old entity, then we don't contain the entity, otherwise
    // check the bounds
    if (_entitiesToDelete.size() > 0) {
        AACube elementCube = element->getAACube();
        foreach(const EntityToDeleteDetails& details, _entitiesToDelete) {
            if (elementCube.contains(details.cube)) {
                containsEntity = true;
                break; // if it contains at least one, we're good to go
            }
        }
    }
    return containsEntity;
}

bool DeleteEntityOperator::preRecursion(OctreeElement* element) {
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
    
    // In Pre-recursion, we're generally deciding whether or not we want to recurse this
    // path of the tree. For this operation, we want to recurse the branch of the tree if
    // and of the following are true:
    //   * We have not yet found the old entity, and this branch contains our old entity
    //   * We have not yet found the new entity, and this branch contains our new entity
    //
    // Note: it's often the case that the branch in question contains both the old entity
    // and the new entity.
    
    bool keepSearching = false; // assume we don't need to search any more
    
    // If we haven't yet found all the entities, and this sub tree contains at least one of our
    // entities, then we need to keep searching.
    if ((_foundCount < _lookingCount) && subTreeContainsSomeEntitiesToDelete(element)) {

        // check against each of our search entities
        foreach(const EntityToDeleteDetails& details, _entitiesToDelete) {

            // If this is the element we're looking for, then ask it to remove the old entity
            // and we can stop searching.
            if (entityTreeElement == details.containingElement) {
                EntityItemID entityItemID = details.entity->getEntityItemID();
                EntityItem* theEntity = entityTreeElement->getEntityWithEntityItemID(entityItemID); // find the actual entity
                entityTreeElement->removeEntityItem(theEntity); // remove it from the element
                _tree->setContainingElement(entityItemID, NULL); // update or id to element lookup
                delete theEntity; // now actually delete the entity!
                _foundCount++;
            }
        }
        
        // if we haven't found all of our search for entities, then keep looking
        keepSearching = (_foundCount < _lookingCount);
    }

    return keepSearching; // if we haven't yet found it, keep looking
}

bool DeleteEntityOperator::postRecursion(OctreeElement* element) {
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
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
    entityTreeElement->pruneChildren(); // take this opportunity to prune any empty leaves
    return keepSearching; // if we haven't yet found it, keep looking
}

