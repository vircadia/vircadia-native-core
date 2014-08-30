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
    bool wantDebug = false;

    if (wantDebug) { 
        qDebug() << "DeleteEntityOperator(EntityTree* tree, const EntityItemID& searchEntityID).... ";
        qDebug() << "    tree=" << tree;
        qDebug() << "    _tree=" << tree;
        qDebug() << "------------ DeleteEntityOperator -- BEFORE WE BEGIN: the tree[" << _tree << "] -------------";
        _tree->dumpTree();
        qDebug() << "------------ DeleteEntityOperator -- END the tree-------------";
    }
    addEntityIDToDeleteList(searchEntityID);
}

DeleteEntityOperator::~DeleteEntityOperator() {
    bool wantDebug = false;

    if (wantDebug) { 
        qDebug() << "~DeleteEntityOperator(EntityTree* tree, const EntityItemID& searchEntityID).... ";
        qDebug() << "------------ ~DeleteEntityOperator -- AFTER WE'RE DONE: the tree[" << _tree << "] -------------";
        _tree->dumpTree();
        qDebug() << "------------ ~DeleteEntityOperator -- END the tree-------------";
    }
}

DeleteEntityOperator::DeleteEntityOperator(EntityTree* tree) :
    _tree(tree),
    _changeTime(usecTimestampNow()),
    _foundCount(0),
    _lookingCount(0)
{
    bool wantDebug = false;

    if (wantDebug) { 
        qDebug() << "DeleteEntityOperator(EntityTree* tree).... ";
        qDebug() << "    tree=" << tree;
        qDebug() << "    _tree=" << _tree;
        qDebug() << "------------ DeleteEntityOperator -- BEFORE WE BEGIN: the tree[" << _tree << "] -------------";
        _tree->dumpTree();
        qDebug() << "------------ DeleteEntityOperator -- END the tree-------------";
    }
}

void DeleteEntityOperator::addEntityIDToDeleteList(const EntityItemID& searchEntityID) {
    bool wantDebug = false;

    if (wantDebug) { 
        qDebug() << "DeleteEntityOperator::addEntityIDToDeleteList(const EntityItemID& searchEntityID).... ";
        qDebug() << "    _tree=" << _tree;
        qDebug() << "    searchEntityID=" << searchEntityID;
    }
 
    // check our tree, to determine if this entity is known
    EntityToDeleteDetails details;
    details.containingElement = _tree->getContainingElement(searchEntityID);

    if (wantDebug) { 
        qDebug() << "    details.containingElement=" << details.containingElement;
    }

    if (details.containingElement) {
        details.entity = details.containingElement->getEntityWithEntityItemID(searchEntityID);

        if (wantDebug) { 
            qDebug() << "    details.entity=" << details.entity;
        }

        if (!details.entity) {
            //assert(false);
            qDebug() << "that's UNEXPECTED, we got a _containingElement, but couldn't find the oldEntity!";
        } else {
            details.cube = details.containingElement->getAACube();

            if (wantDebug) { 
                qDebug() << "    details.cube=" << details.cube;
            }

            _entitiesToDelete << details;
            _lookingCount++;
            
            
            _tree->trackDeletedEntity(searchEntityID);
            // before deleting any entity make sure to remove it from our Mortal, Changing, and Moving lists
            _tree->removeEntityFromSimulationLists(searchEntityID);
        }
    }
    
    if (wantDebug) { 
        qDebug() << "    _entitiesToDelete.size():" << _entitiesToDelete.size();
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
    bool wantDebug = false;

    if (wantDebug) { 
        qDebug() << "DeleteEntityOperator::PreRecursion().... ";
        qDebug() << "    element=" << element;
        qDebug() << "    element.AACube=" << element->getAACube();
        qDebug() << "    _lookingCount=" << _lookingCount;
        qDebug() << "    _foundCount=" << _foundCount;

        qDebug() << "    --------- list of deleting entities -----------";
        foreach(const EntityToDeleteDetails& details, _entitiesToDelete) {

            qDebug() << "    ENTITY TO DELETE";
            qDebug() << "        entity=" << details.entity;
            qDebug() << "        entityItemID=" << details.entity->getEntityItemID();
            qDebug() << "        cube=" << details.cube;
            qDebug() << "        containingElement=" << details.containingElement;
            qDebug() << "        containingElement->getAACube()=" << details.containingElement->getAACube();
        }
        qDebug() << "    --------- list of deleting entities -----------";
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
                bool removed = entityTreeElement->removeEntityItem(theEntity); // remove it from the element
                _tree->setContainingElement(entityItemID, NULL); // update or id to element lookup
                delete theEntity; // now actually delete the entity!
                _foundCount++;

                if (wantDebug) { 
                    qDebug() << "DeleteEntityOperator::PreRecursion().... deleting entity:" << entityItemID;
                    qDebug() << "    details.entity=" << details.entity;
                    qDebug() << "    theEntity=" << theEntity;
                    qDebug() << "    called entityTreeElement->removeEntityItem(theEntity)";
                    qDebug() << "        removed=" << removed;
                    qDebug() << "    called _tree->setContainingElement(entityItemID, NULL)";
                    qDebug() << "    called delete theEntity";
                }
            }
        }
        
        // if we haven't found all of our search for entities, then keep looking
        keepSearching = (_foundCount < _lookingCount);
    }

    return keepSearching; // if we haven't yet found it, keep looking
}

bool DeleteEntityOperator::PostRecursion(OctreeElement* element) {
    bool wantDebug = false;

    if (wantDebug) { 
        qDebug() << "DeleteEntityOperator::PostRecursion().... ";
        qDebug() << "    element=" << element;
        qDebug() << "    element.AACube=" << element->getAACube();
        qDebug() << "    _lookingCount=" << _lookingCount;
        qDebug() << "    _foundCount=" << _foundCount;

        qDebug() << "    --------- list of deleting entities -----------";
        foreach(const EntityToDeleteDetails& details, _entitiesToDelete) {

            qDebug() << "    DELETING ENTITY";
            qDebug() << "        entity=" << details.entity;
            qDebug() << "        entityItemID=" << details.entity->getEntityItemID();
            qDebug() << "        cube=" << details.cube;
            qDebug() << "        containingElement=" << details.containingElement;
        }
        qDebug() << "    --------- list of deleting entities -----------";
    }

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
    if (somethingPruned && wantDebug) {
        qDebug() << "DeleteEntityOperator::PostRecursion() something pruned!!!";
    }
    
    return keepSearching; // if we haven't yet found it, keep looking
}

