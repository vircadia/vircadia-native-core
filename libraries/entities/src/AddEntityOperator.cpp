//
//  AddEntityOperator.cpp
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

#include "AddEntityOperator.h"

AddEntityOperator::AddEntityOperator(EntityTree* tree, 
                        EntityItem* newEntity) :
    _tree(tree),
    _newEntity(newEntity),
    _foundNew(false),
    _changeTime(usecTimestampNow()),
    _newEntityBox()
{
    // caller must have verified existence of newEntity
    assert(_newEntity);

    _newEntityBox = _newEntity->getAACube().clamp(0.0f, 1.0f);

    //qDebug() << "AddEntityOperator::AddEntityOperator() newEntity=" << newEntity;
    //qDebug() << "   _newEntityBox=" << _newEntityBox;
}

bool AddEntityOperator::PreRecursion(OctreeElement* element) {
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);

    //qDebug() << "AddEntityOperator::PreRecursion() entityTreeElement=" << entityTreeElement;
    //entityTreeElement->debugDump();
    
    // In Pre-recursion, we're generally deciding whether or not we want to recurse this
    // path of the tree. For this operation, we want to recurse the branch of the tree if
    // and of the following are true:
    //   * We have not yet found the location for the new entity, and this branch contains the bounds of the new entity
    
    bool keepSearching = false; // assume we don't need to search any more
    
    // If we haven't yet found the new entity,  and this subTreeContains our new
    // entity, then we need to keep searching.
    if (!_foundNew && element->getAACube().contains(_newEntityBox)) {

        //qDebug() << "this element contains the _newEntityBox..." << _newEntityBox;
    
        // If this element is the best fit for the new entity properties, then add/or update it
        if (entityTreeElement->bestFitBounds(_newEntityBox)) {

            //qDebug() << "this element is the best fit for _newEntityBox..." << _newEntityBox;
            //qDebug() << "calling entityTreeElement->addEntityItem(_newEntity);";
            entityTreeElement->addEntityItem(_newEntity);
            //qDebug() << "calling setContainingElement();";
            _tree->setContainingElement(_newEntity->getEntityItemID(), entityTreeElement);

            //qDebug() << "AddEntityOperator calling setContainingElement... new entityID=" << _newEntity->getEntityItemID();
            //_tree->debugDumpMap();
            
            _foundNew = true;
            keepSearching = false;
        } else {
            keepSearching = true;
        }
    }
    
    return keepSearching; // if we haven't yet found it, keep looking
}

bool AddEntityOperator::PostRecursion(OctreeElement* element) {
    // Post-recursion is the unwinding process. For this operation, while we
    // unwind we want to mark the path as being dirty if we changed it below.
    // We might have two paths, one for the old entity and one for the new entity.
    bool keepSearching = !_foundNew;

    // As we unwind, if we're in either of these two paths, we mark our element
    // as dirty.
    if ((_foundNew && element->getAACube().contains(_newEntityBox))) {
        element->markWithChangedTime();
    }
    return keepSearching; // if we haven't yet found it, keep looking
}

OctreeElement* AddEntityOperator::PossiblyCreateChildAt(OctreeElement* element, int childIndex) { 
    // If we're getting called, it's because there was no child element at this index while recursing.
    // We only care if this happens while still searching for the new entity location.
    // Check to see if 
    if (!_foundNew) {
        float childElementScale = element->getAACube().getScale() / 2.0f; // all of our children will be half our scale
        // if the scale of our desired cube is smaller than our children, then consider making a child
        if (_newEntityBox.getLargestDimension() <= childElementScale) {
            int indexOfChildContainingNewEntity = element->getMyChildContaining(_newEntityBox);
        
            if (childIndex == indexOfChildContainingNewEntity) {
                return element->addChildAtIndex(childIndex);
            }
        }
    }
    return NULL; 
}
