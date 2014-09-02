//
//  MovingEntitiesOperator.cpp
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

#include "MovingEntitiesOperator.h"

MovingEntitiesOperator::MovingEntitiesOperator(EntityTree* tree) :
    _tree(tree),
    _changeTime(usecTimestampNow()),
    _foundOldCount(0),
    _foundNewCount(0),
    _lookingCount(0)
{
}

MovingEntitiesOperator::~MovingEntitiesOperator() {
}


void MovingEntitiesOperator::addEntityToMoveList(EntityItem* entity, const AACube& oldCube, const AACube& newCube) {
    // check our tree, to determine if this entity is known
    EntityToMoveDetails details;
    details.oldContainingElement = _tree->getContainingElement(entity->getEntityItemID());
    details.entity = entity;
    details.oldFound = false;
    details.newFound = false;
    details.oldCube = oldCube;
    details.newCube = newCube;
    details.newBox = newCube.clamp(0.0f, 1.0f);
    _entitiesToMove << details;
    _lookingCount++;
}

// does this entity tree element contain the old entity
bool MovingEntitiesOperator::shouldRecurseSubTree(OctreeElement* element) {
    bool containsEntity = false;

    // If we don't have an old entity, then we don't contain the entity, otherwise
    // check the bounds
    if (_entitiesToMove.size() > 0) {
        AACube elementCube = element->getAACube();
        foreach(const EntityToMoveDetails& details, _entitiesToMove) {
            if (elementCube.contains(details.oldCube) || elementCube.contains(details.newCube)) {
                containsEntity = true;
                break; // if it contains at least one, we're good to go
            }
        }
    }
    return containsEntity;
}

bool MovingEntitiesOperator::PreRecursion(OctreeElement* element) {
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
    
    // In Pre-recursion, we're generally deciding whether or not we want to recurse this
    // path of the tree. For this operation, we want to recurse the branch of the tree if
    // and of the following are true:
    //   * We have not yet found the old entity, and this branch contains our old entity
    //   * We have not yet found the new entity, and this branch contains our new entity
    //
    // Note: it's often the case that the branch in question contains both the old entity
    // and the new entity.
    
    bool keepSearching = (_foundOldCount < _lookingCount) || (_foundNewCount < _lookingCount);

    // If we haven't yet found all the entities, and this sub tree contains at least one of our
    // entities, then we need to keep searching.
    if (keepSearching && shouldRecurseSubTree(element)) {

        // check against each of our search entities
        foreach(const EntityToMoveDetails& details, _entitiesToMove) {

            // If this is one of the old elements we're looking for, then ask it to remove the old entity
            if (!details.oldFound && entityTreeElement == details.oldContainingElement) {
                entityTreeElement->removeEntityItem(details.entity);
                _foundOldCount++;
                //details.oldFound = true; // TODO: would be nice to add this optimization
            }

            // If this element is the best fit for the new bounds of this entity then add the entity to the element
            if (!details.newFound && entityTreeElement->bestFitBounds(details.newCube)) {
                EntityItemID entityItemID = details.entity->getEntityItemID();
                entityTreeElement->addEntityItem(details.entity);
                _tree->setContainingElement(entityItemID, entityTreeElement);
                _foundNewCount++;
                //details.newFound = true; // TODO: would be nice to add this optimization
            }
        }
        // if we haven't found all of our search for entities, then keep looking
        keepSearching = (_foundOldCount < _lookingCount) || (_foundNewCount < _lookingCount);
    }

    return keepSearching; // if we haven't yet found it, keep looking
}

bool MovingEntitiesOperator::PostRecursion(OctreeElement* element) {
    // Post-recursion is the unwinding process. For this operation, while we
    // unwind we want to mark the path as being dirty if we changed it below.
    // We might have two paths, one for the old entity and one for the new entity.
    bool keepSearching = (_foundOldCount < _lookingCount) || (_foundNewCount < _lookingCount);

    // As we unwind, if we're in either of these two paths, we mark our element
    // as dirty.
    if ((shouldRecurseSubTree(element))) {
        element->markWithChangedTime();
    }
    
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
    entityTreeElement->pruneChildren(); // take this opportunity to prune any empty leaves

    return keepSearching; // if we haven't yet found it, keep looking
}

OctreeElement* MovingEntitiesOperator::PossiblyCreateChildAt(OctreeElement* element, int childIndex) {
    // If we're getting called, it's because there was no child element at this index while recursing.
    // We only care if this happens while still searching for the new entity locations.
    if (_foundNewCount < _lookingCount) {

        float childElementScale = element->getAACube().getScale() / 2.0f; // all of our children will be half our scale
    
        // check against each of our entities
        foreach(const EntityToMoveDetails& details, _entitiesToMove) {
            EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
            
            // if the scale of our desired cube is smaller than our children, then consider making a child
            if (details.newBox.getLargestDimension() <= childElementScale) {

                int indexOfChildContainingNewEntity = element->getMyChildContaining(details.newBox);
            
                // If the childIndex we were asked if we wanted to create contains this newCube,
                // then we will create this branch and continue. We can exit this loop immediately
                // because if we need this branch for any one entity then it doesn't matter if it's
                // needed for more entities.
                if (childIndex == indexOfChildContainingNewEntity) {
                    OctreeElement* newChild = element->addChildAtIndex(childIndex);
                    return newChild;
                }
            }
        }
    }
    return NULL; 
}
