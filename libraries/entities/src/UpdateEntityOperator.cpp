//
//  UpdateEntityOperator.cpp
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

#include "UpdateEntityOperator.h"

UpdateEntityOperator::UpdateEntityOperator(EntityTree* tree, 
                        EntityTreeElement* containingElement, 
                        EntityItem* existingEntity, 
                        const EntityItemProperties& properties) :
    _tree(tree),
    _existingEntity(existingEntity),
    _containingElement(containingElement),
    _containingElementCube(containingElement->getAACube()), 
    _properties(properties),
    _entityItemID(existingEntity->getEntityItemID()),
    _foundOld(false),
    _foundNew(false),
    _removeOld(false),
    _dontMove(false), // assume we'll be moving
    _changeTime(usecTimestampNow()),
    _oldEntityCube(),
    _newEntityCube(),
    _wantDebug(false)
{
    // caller must have verified existence of containingElement and oldEntity
    assert(_containingElement && _existingEntity);
    
    if (_wantDebug) {
        qDebug() << "UpdateEntityOperator::UpdateEntityOperator() -----------------------------";
    }    

    // Here we have a choice to make, do we want to "tight fit" the actual minimum for the
    // entity into the the element, or do we want to use the entities "relaxed" bounds
    // which can handle all potential rotations?
    // the getMaximumAACube is the relaxed form.
    _oldEntityCube = _existingEntity->getMaximumAACube();
    _oldEntityBox = _oldEntityCube.clamp(0.0f, 1.0f); // clamp to domain bounds

    // If the old properties doesn't contain the properties required to calculate a bounding box,
    // get them from the existing entity. Registration point is required to correctly calculate
    // the bounding box.
    if (!_properties.registrationPointChanged()) {
        _properties.setRegistrationPoint(_existingEntity->getRegistrationPoint());
    }
    
    // If the new properties has position OR dimension changes, but not both, we need to
    // get the old property value and set it in our properties in order for our bounds
    // calculations to work.
    if (_properties.containsPositionChange() && !_properties.containsDimensionsChange()) {
        glm::vec3 oldDimensionsInMeters = _existingEntity->getDimensions() * (float)TREE_SCALE;
        _properties.setDimensions(oldDimensionsInMeters);

        if (_wantDebug) {
            qDebug() << "    ** setting properties dimensions - had position change, no dimension change **";
        }

    }
    if (!_properties.containsPositionChange() && _properties.containsDimensionsChange()) {
        glm::vec3 oldPositionInMeters = _existingEntity->getPosition() * (float)TREE_SCALE;
        _properties.setPosition(oldPositionInMeters);

        if (_wantDebug) {
            qDebug() << "    ** setting properties position - had dimensions change, no position change **";
        }
    }

    // If our new properties don't have bounds details (no change to position, etc) or if this containing element would 
    // be the best fit for our new properties, then just do the new portion of the store pass, since the change path will 
    // be the same for both parts of the update
    bool oldElementBestFit = _containingElement->bestFitBounds(_properties);
    
    // if we don't have bounds properties, then use our old clamped box to determine best fit
    if (!_properties.containsBoundsProperties()) {
        oldElementBestFit = _containingElement->bestFitBounds(_oldEntityBox);

        if (_wantDebug) {
            qDebug() << "    ** old Element best fit - no dimensions change, no position change **";
        }

    }
    
    // For some reason we've seen a case where the original containing element isn't a best fit for the old properties
    // in this case we want to move it, even if the properties haven't changed.
    if (!_properties.containsBoundsProperties() && !oldElementBestFit) {
        _newEntityCube = _oldEntityCube;
        _removeOld = true; // our properties are going to move us, so remember this for later processing

        if (_wantDebug) {
            qDebug() << "    **** UNUSUAL CASE ****  no changes, but not best fit... consider it a move.... **";
        }


    } else if (!_properties.containsBoundsProperties() || oldElementBestFit) {
        _foundOld = true;
        _newEntityCube = _oldEntityCube;
        _dontMove = true;

        if (_wantDebug) {
            qDebug() << "    **** TYPICAL NO MOVE CASE ****";
            qDebug() << "        _properties.containsBoundsProperties():" << _properties.containsBoundsProperties();
            qDebug() << "                             oldElementBestFit:" << oldElementBestFit;
        }

    } else {
        _newEntityCube = _properties.getMaximumAACubeInTreeUnits();
        _removeOld = true; // our properties are going to move us, so remember this for later processing

        if (_wantDebug) {
            qDebug() << "    **** TYPICAL MOVE CASE ****";
        }
    }

    _newEntityBox = _newEntityCube.clamp(0.0f, 1.0f); // clamp to domain bounds


    if (_wantDebug) {
        qDebug() << "    _entityItemID:" << _entityItemID;
        qDebug() << "    _containingElementCube:" << _containingElementCube;
        qDebug() << "    _oldEntityCube:" << _oldEntityCube;
        qDebug() << "    _oldEntityBox:" << _oldEntityBox;
        qDebug() << "    _newEntityCube:" << _newEntityCube;
        qDebug() << "    _newEntityBox:" << _newEntityBox;
        qDebug() << "--------------------------------------------------------------------------";
    }

}


UpdateEntityOperator::~UpdateEntityOperator() {
}


// does this entity tree element contain the old entity
bool UpdateEntityOperator::subTreeContainsOldEntity(OctreeElement* element) {

    // We've found cases where the old entity might be placed in an element that is not actually the best fit
    // so when we're searching the tree for the old element, we use the known cube for the known containing element
    bool elementContainsOldBox = element->getAACube().contains(_containingElementCube);

    if (_wantDebug) {
        bool elementContainsOldCube = element->getAACube().contains(_oldEntityCube);
        qDebug() << "UpdateEntityOperator::subTreeContainsOldEntity()....";
        qDebug() << "    element->getAACube()=" << element->getAACube();
        qDebug() << "    _oldEntityCube=" << _oldEntityCube;
        qDebug() << "    _oldEntityBox=" << _oldEntityBox;
        qDebug() << "    elementContainsOldCube=" << elementContainsOldCube;
        qDebug() << "    elementContainsOldBox=" << elementContainsOldBox;
    }
    return elementContainsOldBox;
}

bool UpdateEntityOperator::subTreeContainsNewEntity(OctreeElement* element) {
    bool elementContainsNewBox = element->getAACube().contains(_newEntityBox);

    if (_wantDebug) {
        bool elementContainsNewCube = element->getAACube().contains(_newEntityCube);
        qDebug() << "UpdateEntityOperator::subTreeContainsNewEntity()....";
        qDebug() << "    element->getAACube()=" << element->getAACube();
        qDebug() << "    _newEntityCube=" << _newEntityCube;
        qDebug() << "    _newEntityBox=" << _newEntityBox;
        qDebug() << "    elementContainsNewCube=" << elementContainsNewCube;
        qDebug() << "    elementContainsNewBox=" << elementContainsNewBox;
    }

    return elementContainsNewBox;
}


bool UpdateEntityOperator::preRecursion(OctreeElement* element) {
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

    bool subtreeContainsOld = subTreeContainsOldEntity(element);
    bool subtreeContainsNew = subTreeContainsNewEntity(element);

    if (_wantDebug) {
        qDebug() << "---- UpdateEntityOperator::preRecursion().... ----";
        qDebug() << "    element=" << element->getAACube();
        qDebug() << "    subtreeContainsOld=" << subtreeContainsOld;
        qDebug() << "    subtreeContainsNew=" << subtreeContainsNew;
        qDebug() << "    _foundOld=" << _foundOld;
        qDebug() << "    _foundNew=" << _foundNew;
    }

    // If we haven't yet found the old entity, and this subTreeContains our old
    // entity, then we need to keep searching.
    if (!_foundOld && subtreeContainsOld) {

        if (_wantDebug) {
            qDebug() << "    OLD TREE CASE....";
            qDebug() << "    entityTreeElement=" << entityTreeElement;
            qDebug() << "    _containingElement=" << _containingElement;
        }

        // If this is the element we're looking for, then ask it to remove the old entity
        // and we can stop searching.
        if (entityTreeElement == _containingElement) {

            if (_wantDebug) {
                qDebug() << "    *** it's the OLD ELEMENT! ***";
            }

            // If the containgElement IS NOT the best fit for the new entity properties
            // then we need to remove it, and the updateEntity below will store it in the
            // correct element.
            if (_removeOld) {

                if (_wantDebug) {
                    qDebug() << "    *** REMOVING from ELEMENT ***";
                }

                entityTreeElement->removeEntityItem(_existingEntity); // NOTE: only removes the entity, doesn't delete it

                // If we haven't yet found the new location, then we need to 
                // make sure to remove our entity to element map, because for
                // now we're not in that map
                if (!_foundNew) {
                    _tree->setContainingElement(_entityItemID, NULL);

                    if (_wantDebug) {
                        qDebug() << "    *** REMOVING from MAP ***";
                    }

                }
            }
            _foundOld = true;
        } else {
            // if this isn't the element we're looking for, then keep searching
            keepSearching = true;
        }
    }

    // If we haven't yet found the new entity,  and this subTreeContains our new
    // entity, then we need to keep searching.
    if (!_foundNew && subtreeContainsNew) {

        if (_wantDebug) {
            qDebug() << "    NEW TREE CASE....";
            qDebug() << "    entityTreeElement=" << entityTreeElement;
            qDebug() << "    _containingElement=" << _containingElement;
            qDebug() << "    entityTreeElement->bestFitBounds(_newEntityBox)=" << entityTreeElement->bestFitBounds(_newEntityBox);
        }


        // If this element is the best fit for the new entity properties, then add/or update it
        if (entityTreeElement->bestFitBounds(_newEntityBox)) {

            if (_wantDebug) {
                qDebug() << "    *** THIS ELEMENT IS BEST FIT ***";
            }

            // if we are the existing containing element, then we can just do the update of the entity properties
            if (entityTreeElement == _containingElement) {

                if (_wantDebug) {
                    qDebug() << "    *** This is the same OLD ELEMENT ***";
                }
            
                // TODO: We shouldn't be in a remove old case and also be the new best fit. This indicates that
                // we have some kind of a logic error in this operator. But, it can handle it properly by setting
                // the new properties for the entity and moving on. Still going to output a warning that if we
                // see consistently we will want to address this.
                if (_removeOld) {
                    qDebug() << "UNEXPECTED - UpdateEntityOperator - "
                                "we thought we needed to removeOld, but the old entity is our best fit.";
                    _removeOld = false;
                    
                    // if we thought we were supposed to remove the old item, and we already did, then we need
                    // to repair this case.
                    if (_foundOld) {
                        if (_wantDebug) {
                            qDebug() << "    *** REPAIRING PREVIOUS REMOVAL from ELEMENT and MAP ***";
                        }
                        entityTreeElement->addEntityItem(_existingEntity);
                        _tree->setContainingElement(_entityItemID, entityTreeElement);
                    }
                }

                // set the entity properties and mark our element as changed.
                bool somethingChanged = _existingEntity->setProperties(_properties);
                if (_wantDebug) {
                    qDebug() << "    *** set properties ***";
                }
            } else {
                // otherwise, this is an add case.
                entityTreeElement->addEntityItem(_existingEntity);
                bool somethingChanged = _existingEntity->setProperties(_properties); // still need to update the properties!
                _tree->setContainingElement(_entityItemID, entityTreeElement);
                if (_wantDebug) {
                    qDebug() << "    *** ADDING ENTITY to ELEMENT and MAP and SETTING PROPERTIES ***";
                }
            }
            _foundNew = true; // we found the new item!
        } else {
            keepSearching = true;
        }
    }

    if (_wantDebug) {
        qDebug() << "    FINAL --- keepSearching=" << keepSearching;
        qDebug() << "--------------------------------------------------";
    }

    
    return keepSearching; // if we haven't yet found it, keep looking
}

bool UpdateEntityOperator::postRecursion(OctreeElement* element) {
    // Post-recursion is the unwinding process. For this operation, while we
    // unwind we want to mark the path as being dirty if we changed it below.
    // We might have two paths, one for the old entity and one for the new entity.
    bool keepSearching = !_foundOld || !_foundNew;

    bool subtreeContainsOld = subTreeContainsOldEntity(element);
    bool subtreeContainsNew = subTreeContainsNewEntity(element);

    // As we unwind, if we're in either of these two paths, we mark our element
    // as dirty.
    if ((_foundOld && subtreeContainsOld) ||
            (_foundNew && subtreeContainsNew)) {
        element->markWithChangedTime();
    }

    // It's not OK to prune if we have the potential of deleting the original containig element.
    // because if we prune the containing element then new might end up reallocating the same memory later 
    // and that will confuse our logic.
    // 
    // it's ok to prune if:
    // 1) we're not removing the old
    // 2) we are removing the old, but this subtree doesn't contain the old
    // 3) we are removing the old, this subtree contains the old, but this element isn't a direct parent of _containingElement
    if (!_removeOld || !subtreeContainsOld || !element->isParentOf(_containingElement)) {
        EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
        entityTreeElement->pruneChildren(); // take this opportunity to prune any empty leaves
    }
    
    return keepSearching; // if we haven't yet found it, keep looking
}

OctreeElement* UpdateEntityOperator::possiblyCreateChildAt(OctreeElement* element, int childIndex) { 
    // If we're getting called, it's because there was no child element at this index while recursing.
    // We only care if this happens while still searching for the new entity location.
    // Check to see if 
    if (!_foundNew) {
        float childElementScale = element->getScale() / 2.0f; // all of our children will be half our scale
        
        // Note: because the entity's bounds might have been clamped to the domain. We want to check if the
        // bounds of the clamped box would fit in our child elements. It may be the case that the actual
        // bounds of the element would hang outside of the child elements cells.
        bool entityWouldFitInChild = _newEntityBox.getLargestDimension() <= childElementScale;

        // if the scale of our desired cube is smaller than our children, then consider making a child
        if (entityWouldFitInChild) {
            int indexOfChildContainingNewEntity = element->getMyChildContaining(_newEntityBox);

            if (childIndex == indexOfChildContainingNewEntity) {
                return element->addChildAtIndex(childIndex);;
            }
        }
    }
    return NULL; 
}
