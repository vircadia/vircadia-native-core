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

bool DeleteEntityOperator::PreRecursion(OctreeElement* element) {
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

                qDebug() << "DeleteEntityOperator::PreRecursion().... details.entity->getEntityItemID()=" << details.entity->getEntityItemID();
                EntityTreeElement* containingElement = _tree->getContainingElement(details.entity->getEntityItemID());
                qDebug() << "DeleteEntityOperator::PreRecursion().... BEFORE delete... containingElement=" << containingElement;
                qDebug() << "DeleteEntityOperator::PreRecursion().... BEFORE delete... details.containingElement=" << details.containingElement;

                // This is a good place to delete it!!!
                EntityItemID entityItemID = details.entity->getEntityItemID();

                qDebug() << "DeleteEntityOperator::PreRecursion().... calling... entityTreeElement->getEntityWithEntityItemID(entityItemID);";
                EntityItem* theEntity = entityTreeElement->getEntityWithEntityItemID(entityItemID);
                qDebug() << "     theEntity=" << theEntity;
                qDebug() << "DeleteEntityOperator::PreRecursion().... calling... removeEntityItem(theEntity)";
                bool removed = entityTreeElement->removeEntityItem(theEntity);
                qDebug() << "     removed=" << removed;

                qDebug() << "DeleteEntityOperator::PreRecursion().... calling... _tree->setContainingElement(entityItemID, NULL);";
                _tree->setContainingElement(entityItemID, NULL);

                qDebug() << "DeleteEntityOperator::PreRecursion().... calling... delete theEntity";
                delete theEntity; // now actually delete it!

                _foundCount++;
            }
        }
        
        // if we haven't found all of our search for entities, then keep looking
        keepSearching = (_foundCount < _lookingCount);
    }

    return keepSearching; // if we haven't yet found it, keep looking
}

bool DeleteEntityOperator::PostRecursion(OctreeElement* element) {
    // Post-recursion is the unwinding process. For this operation, while we
    // unwind we want to mark the path as being dirty if we changed it below.
    // We might have two paths, one for the old entity and one for the new entity.
    bool keepSearching = (_foundCount < _lookingCount);

    // As we unwind, if we're in either of these two paths, we mark our element
    // as dirty.
    if ((subTreeContainsSomeEntitiesToDelete(element))) {
        element->markWithChangedTime();
    }

    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
    bool somethingPruned = entityTreeElement->pruneChildren(); // take this opportunity to prune any empty leaves
    bool wantDebug = false;
    if (somethingPruned && wantDebug) {
        qDebug() << "DeleteEntityOperator::PostRecursion() something pruned!!!";
    }
    
    return keepSearching; // if we haven't yet found it, keep looking
}

