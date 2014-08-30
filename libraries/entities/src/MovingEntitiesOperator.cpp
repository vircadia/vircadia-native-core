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
    bool wantDebug = false;

    if (wantDebug) { 
        qDebug() << "MovingEntitiesOperator..... *********** start here!";
        qDebug() << "    tree=" << tree;
        qDebug() << "    _tree=" << _tree;
    
        qDebug() << "------------ MovingEntitiesOperator -- BEFORE WE BEGIN: the tree[" << _tree << "] -------------";
        _tree->dumpTree();
        qDebug() << "------------ MovingEntitiesOperator -- END the tree-------------";
    }
}

MovingEntitiesOperator::~MovingEntitiesOperator() {
    bool wantDebug = false;

    if (wantDebug) { 
        qDebug() << "~MovingEntitiesOperator().... ";
        qDebug() << "------------ ~MovingEntitiesOperator -- AFTER WE'RE DONE: the tree[" << _tree << "] -------------";
        _tree->dumpTree();
        qDebug() << "------------ ~MovingEntitiesOperator -- END the tree-------------";
    }
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
    bool wantDebug = false;

    if (wantDebug) { 
        qDebug() << "MovingEntitiesOperator::PreRecursion().... ";
        qDebug() << "    element=" << element;
        qDebug() << "    element.AACube=" << element->getAACube();
        qDebug() << "    _lookingCount=" << _lookingCount;
        qDebug() << "    _foundNewCount=" << _foundNewCount;
        qDebug() << "    _foundOldCount=" << _foundOldCount;

        qDebug() << "    --------- list of moving entities -----------";
        foreach(const EntityToMoveDetails& details, _entitiesToMove) {

            qDebug() << "    MOVING ENTITY";
            qDebug() << "        entity=" << details.entity;
            qDebug() << "        entityItemID=" << details.entity->getEntityItemID();
            qDebug() << "        oldCube=" << details.oldCube;
            qDebug() << "        newCube=" << details.newCube;
            qDebug() << "        newBox=" << details.newBox;
            qDebug() << "        oldContainingElement=" << details.oldContainingElement;
            qDebug() << "        oldFound=" << details.oldFound;
            qDebug() << "        newFound=" << details.newFound;
        }
        qDebug() << "    --------- list of moving entities -----------";
    }

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

    if (wantDebug) { 
        qDebug() << "    keepSearching=" << keepSearching;
        qDebug() << "    shouldRecurseSubTree(element)=" << shouldRecurseSubTree(element);
    }
        
    // If we haven't yet found all the entities, and this sub tree contains at least one of our
    // entities, then we need to keep searching.
    if (keepSearching && shouldRecurseSubTree(element)) {

        if (wantDebug) { 
            qDebug() << "    PROCESSING MOVE ON THIS ELEMENT";
        }

        // check against each of our search entities
        
        if (wantDebug) { 
            qDebug() << "    --------- PROCESSING list of moving entities -----------";
        }
        foreach(const EntityToMoveDetails& details, _entitiesToMove) {

            if (wantDebug) { 
                qDebug() << "    PROCESSING --- MOVING ENTITY";
                qDebug() << "        entity=" << details.entity;
                qDebug() << "        entityItemID=" << details.entity->getEntityItemID();
                qDebug() << "        oldCube=" << details.oldCube;
                qDebug() << "        newCube=" << details.newCube;
                qDebug() << "        newBox=" << details.newBox;
                qDebug() << "        oldContainingElement=" << details.oldContainingElement;
                qDebug() << "        oldFound=" << details.oldFound;
                qDebug() << "        newFound=" << details.newFound;

                if (!details.oldFound) {
                    qDebug() << "        THIS ENTITY'S OLD LOCATION HAS NOT BEEN FOUND... ";
                }

                if (entityTreeElement == details.oldContainingElement) {
                    qDebug() << "        THIS ELEMENT IS THE ENTITY'S OLD LOCATION... ";
                }

            }

            // If this is one of the old elements we're looking for, then ask it to remove the old entity
            if (!details.oldFound && entityTreeElement == details.oldContainingElement) {

                if (wantDebug) { 
                    qDebug() << "        PROCESSING REMOVE ENTITY FROM OLD ELEMENT <<<<<<<<<<<<<";
                }

                entityTreeElement->removeEntityItem(details.entity);
                if (wantDebug) { 
                    qDebug() << "removing entityItem from element... ";
                    qDebug() << "    entity=" << details.entity;
                    qDebug() << "    entityItemID=" << details.entity->getEntityItemID();
                    qDebug() << "    entityTreeElement=" << entityTreeElement;
                    qDebug() << "    element=" << element;
                    qDebug() << "    element->getAACube()=" << element->getAACube();
                }

                _foundOldCount++;
                //details.oldFound = true; // TODO: would be nice to add this optimization
            }

            // If this element is the best fit for the new bounds of this entity then add the entity to the element
            if (wantDebug) { 
                bool bestFitCube = entityTreeElement->bestFitBounds(details.newCube);
                bool bestFitBox = entityTreeElement->bestFitBounds(details.newBox);
                qDebug() << "    bestFitCube=" << bestFitCube;
                qDebug() << "    bestFitBox=" << bestFitBox;
                if (bestFitCube != bestFitBox) {
                    qDebug() << "    WHOA!!!! bestFitCube != bestFitBox!!!!";
                }
                if (!details.newFound) {
                    qDebug() << "        THIS ENTITY'S NEW LOCATION HAS NOT BEEN FOUND... ";
                }

                if (entityTreeElement->bestFitBounds(details.newCube)) {
                    qDebug() << "        THIS ELEMENT IS THE ENTITY'S BEST FIT NEW LOCATION... ";
                }
            }

            if (!details.newFound && entityTreeElement->bestFitBounds(details.newCube)) {

                EntityItemID entityItemID = details.entity->getEntityItemID();
                if (wantDebug) { 
                    qDebug() << "        PROCESSING ADD ENTITY TO NEW ELEMENT <<<<<<<<<<<<<";
                    qDebug() << "adding entityItem to element...";
                    qDebug() << "    entity=" << details.entity;
                    qDebug() << "    entityItemID=" << entityItemID;
                    qDebug() << "    entityTreeElement=" << entityTreeElement;
                    qDebug() << "    element=" << element;
                    qDebug() << "    element->getAACube()=" << element->getAACube();
                }
                entityTreeElement->addEntityItem(details.entity);
                _tree->setContainingElement(entityItemID, entityTreeElement);
                
                _foundNewCount++;
                //details.newFound = true; // TODO: would be nice to add this optimization
            }
        }
        if (wantDebug) { 
            qDebug() << "    --------- DONE PROCESSING list of moving entities -----------";
        }
        
        // if we haven't found all of our search for entities, then keep looking
        keepSearching = (_foundOldCount < _lookingCount) || (_foundNewCount < _lookingCount);
    }

    return keepSearching; // if we haven't yet found it, keep looking
}

bool MovingEntitiesOperator::PostRecursion(OctreeElement* element) {
    bool wantDebug = false;

    if (wantDebug) { 
        qDebug() << "MovingEntitiesOperator::PostRecursion().... ";
        qDebug() << "    element=" << element;
        qDebug() << "    element.AACube=" << element->getAACube();
        qDebug() << "    _lookingCount=" << _lookingCount;
        qDebug() << "    _foundNewCount=" << _foundNewCount;
        qDebug() << "    _foundOldCount=" << _foundOldCount;

        qDebug() << "    --------- list of moving entities -----------";
        foreach(const EntityToMoveDetails& details, _entitiesToMove) {

            qDebug() << "    MOVING ENTITY";
            qDebug() << "        entity=" << details.entity;
            qDebug() << "        entityItemID=" << details.entity->getEntityItemID();
            qDebug() << "        oldCube=" << details.oldCube;
            qDebug() << "        newCube=" << details.newCube;
            qDebug() << "        newBox=" << details.newBox;
            qDebug() << "        oldContainingElement=" << details.oldContainingElement;
            qDebug() << "        oldFound=" << details.oldFound;
            qDebug() << "        newFound=" << details.newFound;
        }
        qDebug() << "    --------- list of moving entities -----------";

    }

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
    bool somethingPruned = entityTreeElement->pruneChildren(); // take this opportunity to prune any empty leaves
    if (somethingPruned && wantDebug) {
        qDebug() << "MovingEntitiesOperator::PostRecursion() something pruned!!!";
    }

    return keepSearching; // if we haven't yet found it, keep looking
}

OctreeElement* MovingEntitiesOperator::PossiblyCreateChildAt(OctreeElement* element, int childIndex) {
    bool wantDebug = false;

    if (wantDebug) { 
        qDebug() << "MovingEntitiesOperator::PossiblyCreateChildAt().... ";
        qDebug() << "    element=" << element;
        qDebug() << "    element.AACube=" << element->getAACube();
        qDebug() << "    childIndex=" << childIndex;
        qDebug() << "    _lookingCount=" << _lookingCount;
        qDebug() << "    _foundNewCount=" << _foundNewCount;
        qDebug() << "    _foundOldCount=" << _foundOldCount;

        qDebug() << "    --------- list of moving entities -----------";
        foreach(const EntityToMoveDetails& details, _entitiesToMove) {

            qDebug() << "    MOVING ENTITY";
            qDebug() << "        entity=" << details.entity;
            qDebug() << "        entityItemID=" << details.entity->getEntityItemID();
            qDebug() << "        oldCube=" << details.oldCube;
            qDebug() << "        newCube=" << details.newCube;
            qDebug() << "        newBox=" << details.newBox;
            qDebug() << "        oldContainingElement=" << details.oldContainingElement;
            qDebug() << "        oldFound=" << details.oldFound;
            qDebug() << "        newFound=" << details.newFound;
        }
        qDebug() << "    --------- list of moving entities -----------";
    }
    
    // If we're getting called, it's because there was no child element at this index while recursing.
    // We only care if this happens while still searching for the new entity locations.
    if (_foundNewCount < _lookingCount) {

        float childElementScale = element->getAACube().getScale() / 2.0f; // all of our children will be half our scale
    
        // check against each of our entities
        foreach(const EntityToMoveDetails& details, _entitiesToMove) {
            EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
            
            if (wantDebug) { 
                bool thisElementIsBestFitCube = entityTreeElement->bestFitBounds(details.newCube);
                bool thisElementIsBestFitBox = entityTreeElement->bestFitBounds(details.newBox);
                qDebug() << "    thisElementIsBestFitCube=" << thisElementIsBestFitCube;
                qDebug() << "    thisElementIsBestFitBox=" << thisElementIsBestFitBox;
                qDebug() << "    details.newCube=" << details.newCube;
                qDebug() << "    details.newBox=" << details.newBox;
                qDebug() << "    element=" << element;
                qDebug() << "    element->getAACube()=" << element->getAACube();
            }
            
            // if the scale of our desired cube is smaller than our children, then consider making a child
            if (details.newBox.getLargestDimension() <= childElementScale) {

                int indexOfChildContainingNewEntity = element->getMyChildContaining(details.newBox);
                if (wantDebug) { 
                    qDebug() << "          called element->getMyChildContaining(details.newBox); ---------";
                    qDebug() << "              childIndex=" << childIndex;
                    qDebug() << "              indexOfChildContainingNewEntity=" << indexOfChildContainingNewEntity;
                }
            
                // If the childIndex we were asked if we wanted to create contains this newCube,
                // then we will create this branch and continue. We can exit this loop immediately
                // because if we need this branch for any one entity then it doesn't matter if it's
                // needed for more entities.
                if (childIndex == indexOfChildContainingNewEntity) {
                    OctreeElement* newChild = element->addChildAtIndex(childIndex);
                    if (wantDebug) { 
                        qDebug() << "              CREATING NEW CHILD --- childIndex=" << childIndex << "newChild=" << newChild;
                    }
                    return newChild;
                }
            }
        }
    }
    return NULL; 
}


