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

#include "UpdateEntityOperator.h"

UpdateEntityOperator::UpdateEntityOperator(EntityTreePointer tree,
                        EntityTreeElementPointer containingElement,
                        EntityItemPointer existingEntity,
                        const AACube newQueryAACube) :
    _tree(tree),
    _existingEntity(existingEntity),
    _containingElement(containingElement),
    _containingElementCube(containingElement->getAACube()),
    _entityItemID(existingEntity->getEntityItemID()),
    _foundOld(false),
    _foundNew(false),
    _removeOld(false),
    _changeTime(usecTimestampNow()),
    _oldEntityCube(),
    _newEntityCube(),
    _wantDebug(false)
{
    // caller must have verified existence of containingElement and oldEntity
    assert(_containingElement && _existingEntity);

    if (_wantDebug) {
        qCDebug(entities) << "UpdateEntityOperator::UpdateEntityOperator() -----------------------------";
    }

    // Here we have a choice to make, do we want to "tight fit" the actual minimum for the
    // entity into the the element, or do we want to use the entities "relaxed" bounds
    // which can handle all potential rotations?
    // the getMaximumAACube is the relaxed form.
    _oldEntityCube = _existingEntity->getQueryAACube();
    _oldEntityBox = _oldEntityCube.clamp((float)-HALF_TREE_SCALE, (float)HALF_TREE_SCALE); // clamp to domain bounds

    _newEntityCube = newQueryAACube;
    _newEntityBox = _newEntityCube.clamp((float)-HALF_TREE_SCALE, (float)HALF_TREE_SCALE); // clamp to domain bounds

    // set oldElementBestFit true if the entity was in the correct element before this operator was run.
    bool oldElementBestFit = _containingElement->bestFitBounds(_oldEntityBox);

    // For some reason we've seen a case where the original containing element isn't a best fit for the old properties
    // in this case we want to move it, even if the properties haven't changed.
    if (!oldElementBestFit) {
        _oldEntityBox = _existingEntity->getElement()->getAACube();
        _removeOld = true; // our properties are going to move us, so remember this for later processing

        if (_wantDebug) {
            qCDebug(entities) << "    **** UNUSUAL CASE ****  not best fit.... **";
        }
    }

    if (_wantDebug) {
        qCDebug(entities) << "    _entityItemID:" << _entityItemID;
        qCDebug(entities) << "    _containingElementCube:" << _containingElementCube;
        qCDebug(entities) << "    _oldEntityCube:" << _oldEntityCube;
        qCDebug(entities) << "    _oldEntityBox:" << _oldEntityBox;
        qCDebug(entities) << "    _newEntityCube:" << _newEntityCube;
        qCDebug(entities) << "    _newEntityBox:" << _newEntityBox;
        qCDebug(entities) << "--------------------------------------------------------------------------";
    }

}


UpdateEntityOperator::~UpdateEntityOperator() {
}


// does this entity tree element contain the old entity
bool UpdateEntityOperator::subTreeContainsOldEntity(const OctreeElementPointer& element) {

    // We've found cases where the old entity might be placed in an element that is not actually the best fit
    // so when we're searching the tree for the old element, we use the known cube for the known containing element
    bool elementContainsOldBox = element->getAACube().contains(_containingElementCube);

    if (_wantDebug) {
        bool elementContainsOldCube = element->getAACube().contains(_oldEntityCube);
        qCDebug(entities) << "UpdateEntityOperator::subTreeContainsOldEntity()....";
        qCDebug(entities) << "    element->getAACube()=" << element->getAACube();
        qCDebug(entities) << "    _oldEntityCube=" << _oldEntityCube;
        qCDebug(entities) << "    _oldEntityBox=" << _oldEntityBox;
        qCDebug(entities) << "    elementContainsOldCube=" << elementContainsOldCube;
        qCDebug(entities) << "    elementContainsOldBox=" << elementContainsOldBox;
    }
    return elementContainsOldBox;
}

bool UpdateEntityOperator::subTreeContainsNewEntity(const OctreeElementPointer& element) {
    bool elementContainsNewBox = element->getAACube().contains(_newEntityBox);

    if (_wantDebug) {
        bool elementContainsNewCube = element->getAACube().contains(_newEntityCube);
        qCDebug(entities) << "UpdateEntityOperator::subTreeContainsNewEntity()....";
        qCDebug(entities) << "    element->getAACube()=" << element->getAACube();
        qCDebug(entities) << "    _newEntityCube=" << _newEntityCube;
        qCDebug(entities) << "    _newEntityBox=" << _newEntityBox;
        qCDebug(entities) << "    elementContainsNewCube=" << elementContainsNewCube;
        qCDebug(entities) << "    elementContainsNewBox=" << elementContainsNewBox;
    }

    return elementContainsNewBox;
}


bool UpdateEntityOperator::preRecursion(const OctreeElementPointer& element) {
    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);

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
        qCDebug(entities) << "---- UpdateEntityOperator::preRecursion().... ----";
        qCDebug(entities) << "    element=" << element->getAACube();
        qCDebug(entities) << "    subtreeContainsOld=" << subtreeContainsOld;
        qCDebug(entities) << "    subtreeContainsNew=" << subtreeContainsNew;
        qCDebug(entities) << "    _foundOld=" << _foundOld;
        qCDebug(entities) << "    _foundNew=" << _foundNew;
    }

    // If we haven't yet found the old element, and this subTreeContains our old element,
    // then we need to keep searching.
    if (!_foundOld && subtreeContainsOld) {

        if (_wantDebug) {
            qCDebug(entities) << "    OLD TREE CASE....";
            qCDebug(entities) << "    entityTreeElement=" << entityTreeElement.get();
            qCDebug(entities) << "    _containingElement=" << _containingElement.get();
        }

        // If this is the element we're looking for, then ask it to remove the old entity
        // and we can stop searching.
        if (entityTreeElement == _containingElement) {

            if (_wantDebug) {
                qCDebug(entities) << "    *** it's the OLD ELEMENT! ***";
            }

            // If the containgElement IS NOT the best fit for the new entity properties
            // then we need to remove it, and the updateEntity below will store it in the
            // correct element.
            if (_removeOld) {

                if (_wantDebug) {
                    qCDebug(entities) << "    *** REMOVING from ELEMENT ***";
                }

                // the entity knows what element it's in, so we remove it from that one
                // NOTE: we know we haven't yet added it to its new element because _removeOld is true
                EntityTreeElementPointer oldElement = _existingEntity->getElement();
                oldElement->removeEntityItem(_existingEntity);

                if (oldElement != _containingElement) {
                    qCDebug(entities) << "WARNING entity moved during UpdateEntityOperator recursion";
                    _containingElement->removeEntityItem(_existingEntity);
                }

                if (_wantDebug) {
                    qCDebug(entities) << "    *** REMOVING from MAP ***";
                }
            }
            _foundOld = true;
        } else {
            // if this isn't the element we're looking for, then keep searching
            keepSearching = true;
        }
    }

    // If we haven't yet found the new element, and this subTreeContains our new element,
    // then we need to keep searching.
    if (!_foundNew && subtreeContainsNew) {

        if (_wantDebug) {
            qCDebug(entities) << "    NEW TREE CASE....";
            qCDebug(entities) << "    entityTreeElement=" << entityTreeElement.get();
            qCDebug(entities) << "    _containingElement=" << _containingElement.get();
            qCDebug(entities) << "    entityTreeElement->bestFitBounds(_newEntityBox)="
                              << entityTreeElement->bestFitBounds(_newEntityBox);
        }

        // If this element is the best fit for the new entity properties, then add/or update it
        if (entityTreeElement->bestFitBounds(_newEntityBox)) {

            if (_wantDebug) {
                qCDebug(entities) << "    *** THIS ELEMENT IS BEST FIT ***";
            }

            EntityTreeElementPointer oldElement = _existingEntity->getElement();
            // if we are the existing containing element, then we can just do the update of the entity properties
            if (entityTreeElement == oldElement) {
                if (_wantDebug) {
                    qCDebug(entities) << "    *** This is the same OLD ELEMENT ***";
                }
            } else {
                // otherwise, this is an add case.
                if (oldElement) {
                    oldElement->removeEntityItem(_existingEntity);
                    if (oldElement != _containingElement) {
                        qCDebug(entities) << "WARNING entity moved during UpdateEntityOperator recursion";
                    }
                }
                entityTreeElement->addEntityItem(_existingEntity);
            }
            _foundNew = true; // we found the new element
            _removeOld = false; // and it has already been removed from the old
        } else {
            keepSearching = true;
        }
    }

    if (_wantDebug) {
        qCDebug(entities) << "    FINAL --- keepSearching=" << keepSearching;
        qCDebug(entities) << "--------------------------------------------------";
    }

    return keepSearching; // if we haven't yet found it, keep looking
}

bool UpdateEntityOperator::postRecursion(const OctreeElementPointer& element) {
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
        EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
        entityTreeElement->pruneChildren(); // take this opportunity to prune any empty leaves
    }

    return keepSearching; // if we haven't yet found it, keep looking
}

OctreeElementPointer UpdateEntityOperator::possiblyCreateChildAt(const OctreeElementPointer& element, int childIndex) {
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
