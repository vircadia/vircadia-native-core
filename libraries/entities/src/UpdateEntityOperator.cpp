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
    _properties(properties),
    _entityItemID(existingEntity->getEntityItemID()),
    _foundOld(false),
    _foundNew(false),
    _removeOld(false),
    _changeTime(usecTimestampNow()),
    _oldEntityCube(),
    _newEntityCube()
{
    bool wantDebug = false;
    
    // caller must have verified existence of containingElement and oldEntity
    assert(_containingElement && _existingEntity);

    _oldEntityCube = _existingEntity->getAACube();
    _oldEntityBox = _oldEntityCube.clamp(0.0f, 1.0f); // clamp to domain bounds
    
    if (wantDebug) {
        qDebug() << "UpdateEntityOperator....";
        qDebug() << "    _oldEntityCube=" << _oldEntityCube;
        qDebug() << "    _oldEntityBox=" << _oldEntityBox;
        qDebug() << "    _existingEntity->getPosition()=" << _existingEntity->getPosition() * (float)TREE_SCALE << "in meters";
        qDebug() << "    _existingEntity->getRadius()=" << _existingEntity->getRadius() * (float)TREE_SCALE << "in meters";
    }
    
    // If the new properties has position OR radius changes, but not both, we need to
    // get the old property value and set it in our properties in order for our bounds
    // calculations to work.
    if (_properties.containsPositionChange() && !_properties.containsRadiusChange()) {
        float oldRadiusInMeters = _existingEntity->getRadius() * (float)TREE_SCALE;
        _properties.setRadius(oldRadiusInMeters);
    }

    if (!_properties.containsPositionChange() && _properties.containsRadiusChange()) {
        glm::vec3 oldPositionInMeters = _existingEntity->getPosition() * (float)TREE_SCALE;
        _properties.setPosition(oldPositionInMeters);
    }

    // If our new properties don't have bounds details (no change to position, etc) or if this containing element would 
    // be the best fit for our new properties, then just do the new portion of the store pass, since the change path will 
    // be the same for both parts of the update
    
    bool oldElementBestFit = _containingElement->bestFitBounds(_properties);

    if (wantDebug) {
        qDebug() << "    oldElementBestFit=" << oldElementBestFit;
    }
    
    if (!_properties.containsBoundsProperties() || oldElementBestFit) {
        _foundOld = true;
        _newEntityCube = _oldEntityCube;
        if (wantDebug) {
            qDebug() << "    old element is best element <<<<<";
        }
    } else {
        _newEntityCube = _properties.getAACubeTreeUnits();
        _removeOld = true; // our properties are going to move us, so remember this for later processing
        if (wantDebug) {
            qDebug() << "    we need new element <<<<<";
        }
    }

    _newEntityBox = _newEntityCube.clamp(0.0f, 1.0f); // clamp to domain bounds

    if (wantDebug) {
        qDebug() << "    _properties.getPosition()=" << _properties.getPosition() << "in meters";
        qDebug() << "    _properties.getRadius()=" << _properties.getRadius() << "in meters";
        qDebug() << "    _removeOld=" << _removeOld;
        qDebug() << "    _newEntityCube=" << _newEntityCube;
        qDebug() << "    _newEntityBox=" << _newEntityBox;
        qDebug() << "    _removeOld=" << _removeOld;
    }
}

// does this entity tree element contain the old entity
bool UpdateEntityOperator::subTreeContainsOldEntity(OctreeElement* element) {
    bool elementContainsOldBox = element->getAACube().contains(_oldEntityBox);

    /*
    bool elementContainsOldCube = element->getAACube().contains(_oldEntityCube);
    qDebug() << "UpdateEntityOperator::subTreeContainsOldEntity()....";
    qDebug() << "    element->getAACube()=" << element->getAACube();
    qDebug() << "    _oldEntityCube=" << _oldEntityCube;
    qDebug() << "    _oldEntityBox=" << _oldEntityBox;
    qDebug() << "    elementContainsOldCube=" << elementContainsOldCube;
    qDebug() << "    elementContainsOldBox=" << elementContainsOldBox;
    */

    return elementContainsOldBox;
}

bool UpdateEntityOperator::subTreeContainsNewEntity(OctreeElement* element) {
    bool elementContainsNewBox = element->getAACube().contains(_newEntityBox);

    /*
    bool elementContainsNewCube = element->getAACube().contains(_newEntityCube);
    qDebug() << "UpdateEntityOperator::subTreeContainsNewEntity()....";
    qDebug() << "    element->getAACube()=" << element->getAACube();
    qDebug() << "    _newEntityCube=" << _newEntityCube;
    qDebug() << "    _newEntityBox=" << _newEntityBox;
    qDebug() << "    elementContainsNewCube=" << elementContainsNewCube;
    qDebug() << "    elementContainsNewBox=" << elementContainsNewBox;
    */

    return elementContainsNewBox;
}


bool UpdateEntityOperator::PreRecursion(OctreeElement* element) {
    bool wantDebug = false;
    
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

    if (wantDebug) {
        qDebug() << "UpdateEntityOperator::PreRecursion()....";    
        qDebug() << "    _containingElement=" << _containingElement;
        qDebug() << "    _containingElement->getAACube()=" << _containingElement->getAACube();
        qDebug() << "    _foundOld=" << _foundOld;    
        qDebug() << "    _foundNew=" << _foundNew;

        qDebug() << "    entityTreeElement=" << entityTreeElement;
        qDebug() << "    entityTreeElement->getAACube()=" << entityTreeElement->getAACube();
        qDebug() << "    _oldEntityCube=" << _oldEntityCube;
        qDebug() << "    _oldEntityBox=" << _oldEntityBox;
        qDebug() << "    subtreeContainsOld=" << subtreeContainsOld;

        qDebug() << "    _newEntityCube=" << _newEntityCube;
        qDebug() << "    _newEntityBox=" << _newEntityBox;
        qDebug() << "    subtreeContainsNew=" << subtreeContainsNew;
    }
    
    // If we haven't yet found the old entity, and this subTreeContains our old
    // entity, then we need to keep searching.
    if (!_foundOld && subtreeContainsOld) {
    
        if (wantDebug) {
            qDebug() << "    OLD BRANCH....";
        }


        // If this is the element we're looking for, then ask it to remove the old entity
        // and we can stop searching.
        if (entityTreeElement == _containingElement) {

        if (wantDebug) {
            qDebug() << "    OLD BRANCH.... CONTAINING ELEMENT";
        }
        
            // If the containgElement IS NOT the best fit for the new entity properties
            // then we need to remove it, and the updateEntity below will store it in the
            // correct element.
            if (_removeOld) {
                entityTreeElement->removeEntityItem(_existingEntity); // NOTE: only removes the entity, doesn't delete it

                if (wantDebug) {
                    qDebug() << "    OLD BRANCH.... REMOVE ENTITY.... entityTreeElement->removeEntityItem(_existingEntity);";
                }
                
                // If we haven't yet found the new location, then we need to 
                // make sure to remove our entity to element map, because for
                // now we're not in that map
                if (!_foundNew) {
                    _tree->setContainingElement(_entityItemID, NULL);

                    if (wantDebug) {
                        qDebug() << "    OLD BRANCH.... CLEAR CONTAINING ELEMENT... _tree->setContainingElement(_entityItemID, NULL);";
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

        if (wantDebug) {
            qDebug() << "    NEW BRANCH....";
        }
    

        // If this element is the best fit for the new entity properties, then add/or update it
        if (entityTreeElement->bestFitBounds(_newEntityCube)) {

            if (wantDebug) {
                qDebug() << "    NEW BRANCH.... BEST FIT";
            }

            // if we are the existing containing element, then we can just do the update of the entity properties
            if (entityTreeElement == _containingElement) {
                assert(!_removeOld); // We shouldn't be in a remove old case and also be the new best fit

                // set the entity properties and mark our element as changed.
                _existingEntity->setProperties(_properties);

                if (wantDebug) {
                    qDebug() << "    NEW BRANCH.... BEST FIT AND CONTAINING.... NEW=OLD CASE";
                }

            } else {
                // otherwise, this is an add case.
                entityTreeElement->addEntityItem(_existingEntity);
                _existingEntity->setProperties(_properties); // still need to update the properties!
                _tree->setContainingElement(_entityItemID, entityTreeElement);

                if (wantDebug) {
                    qDebug() << "    NEW BRANCH.... ADD ENTITY";
                    qDebug() << "    NEW BRANCH.... SET CONTAINING";
                }
            }
            _foundNew = true; // we found the new item!
        } else {
            keepSearching = true;
        }
    }
    
    return keepSearching; // if we haven't yet found it, keep looking
}

bool UpdateEntityOperator::PostRecursion(OctreeElement* element) {
    // Post-recursion is the unwinding process. For this operation, while we
    // unwind we want to mark the path as being dirty if we changed it below.
    // We might have two paths, one for the old entity and one for the new entity.
    bool keepSearching = !_foundOld || !_foundNew;

    // As we unwind, if we're in either of these two paths, we mark our element
    // as dirty.
    if ((_foundOld && subTreeContainsOldEntity(element)) ||
            (_foundNew && subTreeContainsNewEntity(element))) {
        element->markWithChangedTime();
    }

    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
    bool somethingPruned = entityTreeElement->pruneChildren(); // take this opportunity to prune any empty leaves
    bool wantDebug = false;
    if (somethingPruned && wantDebug) {
        qDebug() << "UpdateEntityOperator::PostRecursion() something pruned!!!";
    }
    
    return keepSearching; // if we haven't yet found it, keep looking
}

OctreeElement* UpdateEntityOperator::PossiblyCreateChildAt(OctreeElement* element, int childIndex) { 
    // If we're getting called, it's because there was no child element at this index while recursing.
    // We only care if this happens while still searching for the new entity location.
    // Check to see if 
    if (!_foundNew) {
    
        float childElementScale = element->getAACube().getScale() / 2.0f; // all of our children will be half our scale
        // if the scale of our desired cube is smaller than our children, then consider making a child
        if (_newEntityCube.getScale() <= childElementScale) {
            //qDebug() << "UpdateEntityOperator::PossiblyCreateChildAt().... calling element->getMyChildContaining(_newEntityCube);";
            int indexOfChildContainingNewEntity = element->getMyChildContaining(_newEntityCube);
        
            if (childIndex == indexOfChildContainingNewEntity) {
                return element->addChildAtIndex(childIndex);
            }
        }
    }
    return NULL; 
}
