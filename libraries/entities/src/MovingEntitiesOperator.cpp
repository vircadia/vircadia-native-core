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
#include "EntitiesLogging.h"

#include "MovingEntitiesOperator.h"

MovingEntitiesOperator::MovingEntitiesOperator(EntityTreePointer tree) :
    _tree(tree),
    _changeTime(usecTimestampNow()),
    _foundOldCount(0),
    _foundNewCount(0),
    _lookingCount(0),
    _wantDebug(false)
{
}

MovingEntitiesOperator::~MovingEntitiesOperator() {
    if (_wantDebug) {
        bool stopExecution = false;
        qCDebug(entities) << "MovingEntitiesOperator::~MovingEntitiesOperator() -----------------------------";
        qCDebug(entities) << "    _lookingCount:" << _lookingCount;
        qCDebug(entities) << "    _foundOldCount:" << _foundOldCount;
        qCDebug(entities) << "    _foundNewCount:" << _foundNewCount;
        if (_foundOldCount < _lookingCount) {
            qCDebug(entities) << "    FAILURE: **** _foundOldCount < _lookingCount ******";
            stopExecution = true;
        }
        if (_foundNewCount < _lookingCount) {
            qCDebug(entities) << "    FAILURE: **** _foundNewCount < _lookingCount ******";
            stopExecution = true;
        }
        qCDebug(entities) << "--------------------------------------------------------------------------";
        if(stopExecution) {
            debug();
            assert(false);
        }
    }
}


void MovingEntitiesOperator::addEntityToMoveList(EntityItemPointer entity, const AACube& newCube) {
    EntityTreeElementPointer oldContainingElement = entity->getElement();
    AABox newCubeClamped = newCube.clamp((float)-HALF_TREE_SCALE, (float)HALF_TREE_SCALE);

    if (_wantDebug) {
        qCDebug(entities) << "MovingEntitiesOperator::addEntityToMoveList() -----------------------------";
        qCDebug(entities) << "    newCube:" << newCube;
        qCDebug(entities) << "    newCubeClamped:" << newCubeClamped;
        if (oldContainingElement) {
            qCDebug(entities) << "    oldContainingElement:" << oldContainingElement->getAACube();
            qCDebug(entities) << "    oldContainingElement->bestFitBounds(newCubeClamped):" 
                            << oldContainingElement->bestFitBounds(newCubeClamped);
        } else {
            qCDebug(entities) << "    WARNING NO OLD CONTAINING ELEMENT!!!";
        }
    }
    
    if (!oldContainingElement) {
            qCDebug(entities) << "UNEXPECTED!!!! attempting to move entity "<< entity->getEntityItemID() 
                            << "that has no containing element. ";
        return; // bail without adding.
    }

    // If the original containing element is the best fit for the requested newCube locations then
    // we don't actually need to add the entity for moving and we can short circuit all this work
    if (!oldContainingElement->bestFitBounds(newCubeClamped)) {
        // check our tree, to determine if this entity is known
        EntityToMoveDetails details;
        details.oldContainingElement = oldContainingElement;
        details.oldContainingElementCube = oldContainingElement->getAACube();
        details.entity = entity;
        details.oldFound = false;
        details.newFound = false;
        details.newCube = newCube;
        details.newCubeClamped = newCubeClamped;
        _entitiesToMove << details;
        _lookingCount++;

        if (_wantDebug) {
            qCDebug(entities) << "MovingEntitiesOperator::addEntityToMoveList() -----------------------------";
            qCDebug(entities) << "    details.entity:" << details.entity->getEntityItemID();
            qCDebug(entities) << "    details.oldContainingElementCube:" << details.oldContainingElementCube;
            qCDebug(entities) << "    details.newCube:" << details.newCube;
            qCDebug(entities) << "    details.newCubeClamped:" << details.newCubeClamped;
            qCDebug(entities) << "    _lookingCount:" << _lookingCount;
            qCDebug(entities) << "--------------------------------------------------------------------------";
        }
    } else {
        if (_wantDebug) {
            qCDebug(entities) << "    oldContainingElement->bestFitBounds(newCubeClamped) IS BEST FIT... NOTHING TO DO";
        }
    }

    if (_wantDebug) {
        qCDebug(entities) << "--------------------------------------------------------------------------";
    }
}

// does this entity tree element contain the old entity
bool MovingEntitiesOperator::shouldRecurseSubTree(const OctreeElementPointer& element) {
    bool containsEntity = false;

    // If we don't have an old entity, then we don't contain the entity, otherwise
    // check the bounds
    if (_entitiesToMove.size() > 0) {
        const AACube& elementCube = element->getAACube();
        int detailIndex = 0;
        foreach(const EntityToMoveDetails& details, _entitiesToMove) {

            if (_wantDebug) {
                qCDebug(entities) << "MovingEntitiesOperator::shouldRecurseSubTree() details["<< detailIndex <<"]-----------------------------";
                qCDebug(entities) << "    element:" << element->getAACube();
                qCDebug(entities) << "    details.entity:" << details.entity->getEntityItemID();
                qCDebug(entities) << "    details.oldContainingElementCube:" << details.oldContainingElementCube;
                qCDebug(entities) << "    details.newCube:" << details.newCube;
                qCDebug(entities) << "    details.newCubeClamped:" << details.newCubeClamped;
                qCDebug(entities) << "    elementCube.contains(details.newCube)" << elementCube.contains(details.newCube);
                qCDebug(entities) << "    elementCube.contains(details.newCubeClamped)" << elementCube.contains(details.newCubeClamped);
                qCDebug(entities) << "--------------------------------------------------------------------------";
            }

            if (elementCube.contains(details.oldContainingElementCube) || elementCube.contains(details.newCubeClamped)) {
                containsEntity = true;
                break; // if it contains at least one, we're good to go
            }
            detailIndex++;
        }
    }
    return containsEntity;
}

bool MovingEntitiesOperator::preRecursion(const OctreeElementPointer& element) {
    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
    
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
        int detailIndex = 0;
        foreach(const EntityToMoveDetails& details, _entitiesToMove) {
        
            if (_wantDebug) {
                qCDebug(entities) << "MovingEntitiesOperator::preRecursion() details["<< detailIndex <<"]-----------------------------";
                qCDebug(entities) << "    entityTreeElement:" << entityTreeElement->getAACube();
                qCDebug(entities) << "    entityTreeElement->bestFitBounds(details.newCube):" << entityTreeElement->bestFitBounds(details.newCube);
                qCDebug(entities) << "    details.entity:" << details.entity->getEntityItemID();
                qCDebug(entities) << "    details.oldContainingElementCube:" << details.oldContainingElementCube;
                qCDebug(entities) << "    entityTreeElement:" << entityTreeElement.get();
                qCDebug(entities) << "    details.newCube:" << details.newCube;
                qCDebug(entities) << "    details.newCubeClamped:" << details.newCubeClamped;
                qCDebug(entities) << "    _lookingCount:" << _lookingCount;
                qCDebug(entities) << "    _foundOldCount:" << _foundOldCount;
                qCDebug(entities) << "--------------------------------------------------------------------------";
            }
        

            // If this is one of the old elements we're looking for, then ask it to remove the old entity
            if (!details.oldFound && entityTreeElement == details.oldContainingElement) {
                // DO NOT remove the entity here.  It will be removed when added to the destination element.
                _foundOldCount++;
                //details.oldFound = true; // TODO: would be nice to add this optimization
                if (_wantDebug) {
                    qCDebug(entities) << "MovingEntitiesOperator::preRecursion() -----------------------------";
                    qCDebug(entities) << "    FOUND OLD - REMOVING";
                    qCDebug(entities) << "    entityTreeElement == details.oldContainingElement";
                    qCDebug(entities) << "--------------------------------------------------------------------------";
                }
            }

            // If this element is the best fit for the new bounds of this entity then add the entity to the element
            if (!details.newFound && entityTreeElement->bestFitBounds(details.newCube)) {
                // remove from the old before adding
                EntityTreeElementPointer oldElement = details.entity->getElement();
                if (oldElement != entityTreeElement) {
                    if (oldElement) {
                        oldElement->removeEntityItem(details.entity);
                    }
                    entityTreeElement->addEntityItem(details.entity);
                }
                _foundNewCount++;
                //details.newFound = true; // TODO: would be nice to add this optimization
                if (_wantDebug) {
                    qCDebug(entities) << "MovingEntitiesOperator::preRecursion() -----------------------------";
                    qCDebug(entities) << "    FOUND NEW - ADDING";
                    qCDebug(entities) << "    entityTreeElement->bestFitBounds(details.newCube)";
                    qCDebug(entities) << "--------------------------------------------------------------------------";
                }
            }
            detailIndex++;
        }
        // if we haven't found all of our search for entities, then keep looking
        keepSearching = (_foundOldCount < _lookingCount) || (_foundNewCount < _lookingCount);
    }

    return keepSearching; // if we haven't yet found it, keep looking
}

bool MovingEntitiesOperator::postRecursion(const OctreeElementPointer& element) {
    // Post-recursion is the unwinding process. For this operation, while we
    // unwind we want to mark the path as being dirty if we changed it below.
    // We might have two paths, one for the old entity and one for the new entity.
    bool keepSearching = (_foundOldCount < _lookingCount) || (_foundNewCount < _lookingCount);

    // As we unwind, if we're in either of these two paths, we mark our element
    // as dirty.
    if ((shouldRecurseSubTree(element))) {
        element->markWithChangedTime();
    }
    


    // It's not OK to prune if we have the potential of deleting the original containing element
    // because if we prune the containing element then new might end up reallocating the same memory later 
    // and that will confuse our logic.
    // 
    // it's ok to prune if:
    // 2) this subtree doesn't contain any old elements
    // 3) this subtree contains an old element, but this element isn't a direct parent of any old containing element

    bool elementSubTreeContainsOldElements = false;
    bool elementIsDirectParentOfOldElment = false;
    foreach(const EntityToMoveDetails& details, _entitiesToMove) {
        if (element->getAACube().contains(details.oldContainingElementCube)) {
            elementSubTreeContainsOldElements = true;
        }
        if (element->isParentOf(details.oldContainingElement)) {
            elementIsDirectParentOfOldElment = true;
        }
    }
    if (!elementSubTreeContainsOldElements || !elementIsDirectParentOfOldElment) {
        EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
        entityTreeElement->pruneChildren(); // take this opportunity to prune any empty leaves
    }

    return keepSearching; // if we haven't yet found it, keep looking
}

OctreeElementPointer MovingEntitiesOperator::possiblyCreateChildAt(const OctreeElementPointer& element, int childIndex) {
    // If we're getting called, it's because there was no child element at this index while recursing.
    // We only care if this happens while still searching for the new entity locations.
    if (_foundNewCount < _lookingCount) {

        float childElementScale = element->getAACube().getScale() / 2.0f; // all of our children will be half our scale
    
        // check against each of our entities
        foreach(const EntityToMoveDetails& details, _entitiesToMove) {

            // if the scale of our desired cube is smaller than our children, then consider making a child
            if (details.newCubeClamped.getLargestDimension() <= childElementScale) {

                int indexOfChildContainingNewEntity = element->getMyChildContaining(details.newCubeClamped);
            
                // If the childIndex we were asked if we wanted to create contains this newCube,
                // then we will create this branch and continue. We can exit this loop immediately
                // because if we need this branch for any one entity then it doesn't matter if it's
                // needed for more entities.
                if (childIndex == indexOfChildContainingNewEntity) {
                    return element->addChildAtIndex(childIndex);
                }
            }
        }
    }
    return NULL; 
}
