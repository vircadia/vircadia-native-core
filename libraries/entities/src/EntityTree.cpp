//
//  EntityTree.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityTree.h"

#include "MovingEntitiesOperator.h"

EntityTree::EntityTree(bool shouldReaverage) : Octree(shouldReaverage) {
    _rootElement = createNewElement();
}

EntityTreeElement* EntityTree::createNewElement(unsigned char * octalCode) {
    EntityTreeElement* newElement = new EntityTreeElement(octalCode);
    newElement->setTree(this);
    return newElement;
}

void EntityTree::eraseAllOctreeElements() {
    _entityToElementMap.clear();
    Octree::eraseAllOctreeElements();
}

bool EntityTree::handlesEditPacketType(PacketType packetType) const {
    // we handle these types of "edit" packets
    switch (packetType) {
        case PacketTypeEntityAddOrEdit:
        case PacketTypeEntityErase:
            return true;
        default:
            return false;
    }
}




/// Give an EntityItemID and EntityItemProperties, this will either find the correct entity that already exists
/// in the tree or it will create a new entity of the type specified by the properties and return that item.
/// In the case that it creates a new item, the item will be properly added to the tree and all appropriate lookup hashes.
EntityItem* EntityTree::getOrCreateEntityItem(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItem* result = NULL;

    // we need to first see if we already have the entity in our tree by finding the containing element of the entity
    EntityTreeElement* containingElement = getContainingElement(entityID);
    if (containingElement) {
        result = containingElement->getEntityWithEntityItemID(entityID);
    }
    
    // if the element does not exist, then create a new one of the specified type...
    if (!result) {
        result = addEntity(entityID, properties);
    }
    return result;
}

class AddEntityOperator : public RecurseOctreeOperator {
public:
    AddEntityOperator(EntityTree* tree, EntityItem* newEntity);
                            
    virtual bool PreRecursion(OctreeElement* element);
    virtual bool PostRecursion(OctreeElement* element);
    virtual OctreeElement* PossiblyCreateChildAt(OctreeElement* element, int childIndex);
private:
    EntityTree* _tree;
    EntityItem* _newEntity;
    bool _foundNew;
    quint64 _changeTime;

    AABox _newEntityBox;
};

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

qDebug() << "AddEntityOperator::AddEntityOperator() newEntity=" << newEntity;
qDebug() << "   _newEntityBox=" << _newEntityBox;
}

bool AddEntityOperator::PreRecursion(OctreeElement* element) {
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);

qDebug() << "AddEntityOperator::PreRecursion() entityTreeElement=" << entityTreeElement;
entityTreeElement->debugDump();
    
    // In Pre-recursion, we're generally deciding whether or not we want to recurse this
    // path of the tree. For this operation, we want to recurse the branch of the tree if
    // and of the following are true:
    //   * We have not yet found the location for the new entity, and this branch contains the bounds of the new entity
    
    bool keepSearching = false; // assume we don't need to search any more
    
    // If we haven't yet found the new entity,  and this subTreeContains our new
    // entity, then we need to keep searching.
    if (!_foundNew && element->getAACube().contains(_newEntityBox)) {

qDebug() << "this element contains the _newEntityBox..." << _newEntityBox;
    
        // If this element is the best fit for the new entity properties, then add/or update it
        if (entityTreeElement->bestFitBounds(_newEntityBox)) {

qDebug() << "this element is the best fit for _newEntityBox..." << _newEntityBox;


qDebug() << "calling entityTreeElement->addEntityItem(_newEntity);";
            entityTreeElement->addEntityItem(_newEntity);
qDebug() << "calling setContainingElement();";
            _tree->setContainingElement(_newEntity->getEntityItemID(), entityTreeElement);
qDebug() << "AddEntityOperator calling setContainingElement... new entityID=" << _newEntity->getEntityItemID();
_tree->debugDumpMap();
            
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
        int indexOfChildContainingNewEntity = element->getMyChildContaining(_newEntityBox);
        
        if (childIndex == indexOfChildContainingNewEntity) {
            return element->addChildAtIndex(childIndex);
        }
    }
    return NULL; 
}

/// Adds a new entity item to the tree
void EntityTree::addEntityItem(EntityItem* entityItem) {
    // You should not call this on existing entities that are already part of the tree! Call updateEntity()
    EntityItemID entityID = entityItem->getEntityItemID();
    EntityTreeElement* containingElement = getContainingElement(entityID);
    assert(containingElement == NULL); // don't call addEntityItem() on existing entity items

    // Recurse the tree and store the entity in the correct tree element
    //qDebug() << "about to call recurseTreeWithOperator(AddEntityOperator)...";
    AddEntityOperator theOperator(this, entityItem);
    recurseTreeWithOperator(&theOperator);
    //qDebug() << "AFTER... about to call recurseTreeWithOperator(AddEntityOperator)...";
    //debugDumpMap();

    // check to see if we need to simulate this entity..
    changeEntityState(entityItem, EntityItem::Static, entityItem->getSimulationState());

    _isDirty = true;
}


class UpdateEntityOperator : public RecurseOctreeOperator {
public:
    UpdateEntityOperator(EntityTree* tree, EntityTreeElement* containingElement, 
                            EntityItem* existingEntity, const EntityItemProperties& properties);
                            
    virtual bool PreRecursion(OctreeElement* element);
    virtual bool PostRecursion(OctreeElement* element);
    virtual OctreeElement* PossiblyCreateChildAt(OctreeElement* element, int childIndex);
private:
    EntityTree* _tree;
    EntityItem* _existingEntity;
    EntityTreeElement* _containingElement;
    EntityItemProperties _properties;
    EntityItemID _entityItemID;
    bool _foundOld;
    bool _foundNew;
    bool _removeOld;
    quint64 _changeTime;

    AACube _oldEntityCube;
    AACube _newEntityCube;

    AABox _oldEntityBox;
    AABox _newEntityBox;

    bool subTreeContainsOldEntity(OctreeElement* element);
    bool subTreeContainsNewEntity(OctreeElement* element);
};

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
    bool elementContainsOldCube = element->getAACube().contains(_oldEntityCube);

    /*
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
    bool elementContainsNewCube = element->getAACube().contains(_newEntityCube);

    /*
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

    /*
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
    bool somethingPruned = entityTreeElement->pruneChildren(); // take this opportunity to prune any empty leaves
    if (somethingPruned) {
        qDebug() << "UpdateEntityOperator::PostRecursion() something pruned!!!";
    }
    */
    
    return keepSearching; // if we haven't yet found it, keep looking
}

OctreeElement* UpdateEntityOperator::PossiblyCreateChildAt(OctreeElement* element, int childIndex) { 
    // If we're getting called, it's because there was no child element at this index while recursing.
    // We only care if this happens while still searching for the new entity location.
    // Check to see if 
    if (!_foundNew) {
        int indexOfChildContainingNewEntity = element->getMyChildContaining(_newEntityCube);
        
        if (childIndex == indexOfChildContainingNewEntity) {
            return element->addChildAtIndex(childIndex);
        }
    }
    return NULL; 
}


bool EntityTree::updateEntity(const EntityItemID& entityID, const EntityItemProperties& properties) {
    // You should not call this on existing entities that are already part of the tree! Call updateEntity()
    EntityTreeElement* containingElement = getContainingElement(entityID);
    if (!containingElement) {
        assert(containingElement); // don't call updateEntity() on entity items that don't exist
        return false;
    }

    EntityItem* existingEntity = containingElement->getEntityWithEntityItemID(entityID);
    if (!existingEntity) {
        assert(existingEntity); // don't call updateEntity() on entity items that don't exist
        return false;
    }
    
    // check to see if we need to simulate this entity...
    EntityItem::SimuationState oldState = existingEntity->getSimulationState();
    
    UpdateEntityOperator theOperator(this, containingElement, existingEntity, properties);
    recurseTreeWithOperator(&theOperator);
    _isDirty = true;

    EntityItem::SimuationState newState = existingEntity->getSimulationState();
    changeEntityState(existingEntity, oldState, newState);

    containingElement = getContainingElement(entityID);
    if (!containingElement) {
        qDebug() << "after updateEntity() we no longer have a containing element???";
        assert(containingElement); // don't call updateEntity() on entity items that don't exist
    }
    
    return true;
}


EntityItem* EntityTree::addEntity(const EntityItemID& entityID, const EntityItemProperties& properties) {

    // NOTE: This method is used in the client and the server tree. In the client, it's possible to create EntityItems 
    // that do not yet have known IDs. In the server tree however we don't want to have entities without known IDs.
    if (getIsServer() && !entityID.isKnownID) {
        //assert(entityID.isKnownID);
        qDebug() << "UNEXPECTED!!! ----- EntityTree::addEntity()... (getIsSever() && !entityID.isKnownID)";
    }

    EntityItem* result = NULL;
    // You should not call this on existing entities that are already part of the tree! Call updateEntity()
    EntityTreeElement* containingElement = getContainingElement(entityID);
    if (containingElement) {
        qDebug() << "UNEXPECTED!!! ----- EntityTree::addEntity()... entityID=" << entityID << "containingElement=" << containingElement;
        assert(containingElement == NULL); // don't call addEntity() on existing entity items
        return result;
    }

    // construct the instance of the entity
    EntityTypes::EntityType type = properties.getType();
    result = EntityTypes::constructEntityItem(type, entityID, properties);

    if (result) {
        // this does the actual adding of the entity
        addEntityItem(result);
    }
    return result;
}


class EntityToDeleteDetails {
public:
    const EntityItem* entity;
    AACube cube;
    EntityTreeElement* containingElement;
};

inline uint qHash(const EntityToDeleteDetails& a, uint seed) {
    return qHash(a.entity->getEntityItemID(), seed);
}

inline bool operator==(const EntityToDeleteDetails& a, const EntityToDeleteDetails& b) {
    return a.entity->getEntityItemID() == b.entity->getEntityItemID();
}

class DeleteEntityOperator : public RecurseOctreeOperator {
public:
    DeleteEntityOperator(EntityTree* tree);
    DeleteEntityOperator(EntityTree* tree, const EntityItemID& searchEntityID);
    void addEntityIDToDeleteList(const EntityItemID& searchEntityID);
    virtual bool PreRecursion(OctreeElement* element);
    virtual bool PostRecursion(OctreeElement* element);
private:
    EntityTree* _tree;
    QSet<EntityToDeleteDetails> _entitiesToDelete;
    quint64 _changeTime;
    int _foundCount;
    int _lookingCount;
    bool subTreeContainsSomeEntitiesToDelete(OctreeElement* element);
};

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

                // This is a good place to delete it!!!
                EntityItemID entityItemID = details.entity->getEntityItemID();
                //qDebug() << "DeleteEntityOperator::PreRecursion() BEFORE entityTreeElement->removeEntityWithEntityItemID(); element=" << entityTreeElement << "id=" << entityItemID;
                entityTreeElement->removeEntityWithEntityItemID(entityItemID);
                //qDebug() << "DeleteEntityOperator::PreRecursion() AFTER entityTreeElement->removeEntityWithEntityItemID(); element=" << entityTreeElement << "id=" << entityItemID;

                _tree->setContainingElement(entityItemID, NULL);
                //qDebug() << "DeleteEntityOperator calling setContainingElement(NULL)... entityID=" << entityItemID;
                //_tree->debugDumpMap();

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

/*
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
    bool somethingPruned = entityTreeElement->pruneChildren(); // take this opportunity to prune any empty leaves
    if (somethingPruned) {
        qDebug() << "DeleteEntityOperator::PostRecursion() something pruned!!!";
    }
 */
    
    return keepSearching; // if we haven't yet found it, keep looking
}

void EntityTree::deleteEntity(const EntityItemID& entityID) {
    // NOTE: callers must lock the tree before using this method
    //EntityTreeElement* containingElement = getContainingElement(entityID);
    //qDebug() << "EntityTree::deleteEntity().... BEFORE DELETE... containingElement=" << containingElement;
    //debugDumpMap();

    // First, look for the existing entity in the tree..
    DeleteEntityOperator theOperator(this, entityID);

    recurseTreeWithOperator(&theOperator);
    _isDirty = true;

    bool wantDebug = true;
    if (wantDebug) {
        EntityTreeElement* containingElement = getContainingElement(entityID);
        qDebug() << "EntityTree::deleteEntity().... after delete... containingElement=" << containingElement;
    }
}

void EntityTree::deleteEntities(QSet<EntityItemID> entityIDs) {
    // NOTE: callers must lock the tree before using this method

    DeleteEntityOperator theOperator(this);
    foreach(const EntityItemID& entityID, entityIDs) {
        // First, look for the existing entity in the tree..
        theOperator.addEntityIDToDeleteList(entityID);
    }

    recurseTreeWithOperator(&theOperator);
    _isDirty = true;

    bool wantDebug = false;
    if (wantDebug) {
        foreach(const EntityItemID& entityID, entityIDs) {
            EntityTreeElement* containingElement = getContainingElement(entityID);
            qDebug() << "EntityTree::deleteEntities().... after delete... entityID=" << entityID 
                        << "containingElement=" << containingElement;
        }
    }
}

/// This method is used to find and fix entity IDs that are shifting from creator token based to known ID based entity IDs. 
/// This should only be used on a client side (viewing) tree. The typical usage is that a local editor has been creating 
/// entities in the local tree, those entities have creatorToken based entity IDs. But those entity edits are also sent up to
/// the server, and the server eventually sends back to the client two messages that can come in varying order. The first 
/// message would be a typical query/viewing data message conversation in which the viewer "sees" the newly created entity. 
/// Those entities that have been seen, will have the authoritative "known ID". Therefore there is a potential that there can 
/// be two copies of the same entity in the tree: the "local only" "creator token" version of the entity and the "seen" 
/// "knownID" version of the entity. The server also sends an "entityAdded" message to the client which contains the mapping 
/// of the creator token to the known ID. These messages can come in any order, so we need to handle the follow cases:
///
///     Case A: The local edit occurs, the addEntity message arrives, the "viewed data" has not yet arrived.
///             In this case, we can expect that our local tree has only one copy of the entity (the creator token), 
///             and we only really need to fix up that entity with a new version of the ID that includes the knownID
///
///     Case B: The local edit occurs, the "viewed data" for the new entity arrives, then the addEntity message arrives.
///             In this case, we can expect that our local tree has two copies of the entity (the creator token, and the
///             known ID version). We end up with two version of the entity because the server sends viewers only the 
///             known ID version without a creator token. And we don't yet know the mapping until we get the mapping message.
///             In this case we need to fix up that entity with a new version of the ID that includes the knownID and
///             we need to delete the extra copy of the entity.
///
/// This method handles both of these cases.
///
/// NOTE: unlike some operations on the tree, this process does not mark the tree as being changed. This is because
/// we're not changing the content of the tree, we're only changing the internal IDs that map entities from creator
/// based to known IDs. This means we don't have to recurse the tree to mark the changed path as dirty.
void EntityTree::handleAddEntityResponse(const QByteArray& packet) {

    //assert(getIsClient()); // we should only call this on client trees
    if (!getIsClient()) {
        qDebug() << "UNEXPECTED!!! EntityTree::handleAddEntityResponse() with !getIsClient() ***";
    }

    const unsigned char* dataAt = reinterpret_cast<const unsigned char*>(packet.data());
    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    int bytesRead = numBytesPacketHeader;
    dataAt += numBytesPacketHeader;

    uint32_t creatorTokenID;
    memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
    dataAt += sizeof(creatorTokenID);
    bytesRead += sizeof(creatorTokenID);

    QUuid entityID = QUuid::fromRfc4122(packet.mid(bytesRead, NUM_BYTES_RFC4122_UUID));
    dataAt += NUM_BYTES_RFC4122_UUID;

    // First, look for the existing entity in the tree..
    EntityItemID searchEntityID;
    searchEntityID.id = entityID;
    searchEntityID.creatorTokenID = creatorTokenID;

    lockForWrite();

    // find the creator token version, it's containing element, and the entity itself    
    EntityItem* foundEntity = NULL;
    EntityItemID  creatorTokenVersion = searchEntityID.convertToCreatorTokenVersion();
    EntityItemID  knownIDVersion = searchEntityID.convertToKnownIDVersion();

    bool wantDebug = false;
    if (wantDebug) {
        qDebug() << "EntityTree::handleAddEntityResponse()..."; 
        qDebug() << "    creatorTokenID=" << creatorTokenID;
        qDebug() << "    entityID=" << entityID;
        qDebug() << "    searchEntityID=" << searchEntityID;
        qDebug() << "    creatorTokenVersion=" << creatorTokenVersion;
        qDebug() << "    knownIDVersion=" << knownIDVersion;
    }

    // First look for and find the "viewed version" of this entity... it's possible we got
    // the known ID version sent to us between us creating our local version, and getting this
    // remapping message. If this happened, we actually want to find and delete that version of
    // the entity.
    EntityTreeElement* knownIDVersionContainingElement = getContainingElement(knownIDVersion);

    if (wantDebug) {
        qDebug() << "    knownIDVersionContainingElement=" << knownIDVersionContainingElement;
    }

    if (knownIDVersionContainingElement) {
        foundEntity = knownIDVersionContainingElement->getEntityWithEntityItemID(knownIDVersion);

        if (wantDebug) {
            qDebug() << "    foundEntity=" << foundEntity;
        }

        if (foundEntity) {
            knownIDVersionContainingElement->removeEntityWithEntityItemID(knownIDVersion);
            setContainingElement(knownIDVersion, NULL);
            
            if (wantDebug) {
                qDebug() << "    FOUND VIEWED VERSION, removing entity, resetting containing element";
            }
            
        }
    }

    EntityTreeElement* creatorTokenContainingElement = getContainingElement(creatorTokenVersion);
    if (wantDebug) {
        qDebug() << "    creatorTokenContainingElement=" << creatorTokenContainingElement;
    }
    if (creatorTokenContainingElement) {
        foundEntity = creatorTokenContainingElement->getEntityWithEntityItemID(creatorTokenVersion);
        if (wantDebug) {
            qDebug() << "    foundEntity=" << foundEntity;
        }
        if (foundEntity) {
            creatorTokenContainingElement->updateEntityItemID(creatorTokenVersion, knownIDVersion);
            setContainingElement(creatorTokenVersion, NULL);
            setContainingElement(knownIDVersion, creatorTokenContainingElement);

            if (wantDebug) {
                qDebug() << "    FOUND CREATOR VERSION, updating entity ID and resetting containing element";
            }

        }
    }
    unlock();
}


class FindNearPointArgs {
public:
    glm::vec3 position;
    float targetRadius;
    bool found;
    const EntityItem* closestEntity;
    float closestEntityDistance;
};


bool EntityTree::findNearPointOperation(OctreeElement* element, void* extraData) {
    FindNearPointArgs* args = static_cast<FindNearPointArgs*>(extraData);
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);

    glm::vec3 penetration;
    bool sphereIntersection = entityTreeElement->getAACube().findSpherePenetration(args->position,
                                                                    args->targetRadius, penetration);

    // If this entityTreeElement contains the point, then search it...
    if (sphereIntersection) {
        const EntityItem* thisClosestEntity = entityTreeElement->getClosestEntity(args->position);

        // we may have gotten NULL back, meaning no entity was available
        if (thisClosestEntity) {
            glm::vec3 entityPosition = thisClosestEntity->getPosition();
            float distanceFromPointToEntity = glm::distance(entityPosition, args->position);

            // If we're within our target radius
            if (distanceFromPointToEntity <= args->targetRadius) {
                // we are closer than anything else we've found
                if (distanceFromPointToEntity < args->closestEntityDistance) {
                    args->closestEntity = thisClosestEntity;
                    args->closestEntityDistance = distanceFromPointToEntity;
                    args->found = true;
                }
            }
        }

        // we should be able to optimize this...
        return true; // keep searching in case children have closer entities
    }

    // if this element doesn't contain the point, then none of it's children can contain the point, so stop searching
    return false;
}

const EntityItem* EntityTree::findClosestEntity(glm::vec3 position, float targetRadius) {
    FindNearPointArgs args = { position, targetRadius, false, NULL, FLT_MAX };
    lockForRead();
    // NOTE: This should use recursion, since this is a spatial operation
    recurseTreeWithOperation(findNearPointOperation, &args);
    unlock();
    return args.closestEntity;
}

class FindAllNearPointArgs {
public:
    glm::vec3 position;
    float targetRadius;
    QVector<const EntityItem*> entities;
};


bool EntityTree::findInSphereOperation(OctreeElement* element, void* extraData) {
    FindAllNearPointArgs* args = static_cast<FindAllNearPointArgs*>(extraData);
    glm::vec3 penetration;
    bool sphereIntersection = element->getAACube().findSpherePenetration(args->position,
                                                                    args->targetRadius, penetration);

    // If this element contains the point, then search it...
    if (sphereIntersection) {
        EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
        entityTreeElement->getEntities(args->position, args->targetRadius, args->entities);
        return true; // keep searching in case children have closer entities
    }

    // if this element doesn't contain the point, then none of it's children can contain the point, so stop searching
    return false;
}

// NOTE: assumes caller has handled locking
void EntityTree::findEntities(const glm::vec3& center, float radius, QVector<const EntityItem*>& foundEntities) {
    FindAllNearPointArgs args = { center, radius };
    // NOTE: This should use recursion, since this is a spatial operation
    recurseTreeWithOperation(findInSphereOperation, &args);

    // swap the two lists of entity pointers instead of copy
    foundEntities.swap(args.entities);
}

class FindEntitiesInCubeArgs {
public:
    FindEntitiesInCubeArgs(const AACube& cube) 
        : _cube(cube), _foundEntities() {
    }

    AACube _cube;
    QVector<EntityItem*> _foundEntities;
};

bool EntityTree::findInCubeOperation(OctreeElement* element, void* extraData) {
    FindEntitiesInCubeArgs* args = static_cast<FindEntitiesInCubeArgs*>(extraData);
    const AACube& elementCube = element->getAACube();
    if (elementCube.touches(args->_cube)) {
        EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
        entityTreeElement->getEntities(args->_cube, args->_foundEntities);
        return true;
    }
    return false;
}

// NOTE: assumes caller has handled locking
void EntityTree::findEntities(const AACube& cube, QVector<EntityItem*> foundEntities) {
    FindEntitiesInCubeArgs args(cube);
    // NOTE: This should use recursion, since this is a spatial operation
    recurseTreeWithOperation(findInCubeOperation, &args);
    // swap the two lists of entity pointers instead of copy
    foundEntities.swap(args._foundEntities);
}

EntityItem* EntityTree::findEntityByID(const QUuid& id) {
    EntityItemID entityID(id);

    bool wantDebug = false;
    if (wantDebug) {
        qDebug() << "EntityTree::findEntityByID()...";
        qDebug() << "    id=" << id;
        qDebug() << "    entityID=" << entityID;
        qDebug() << "_entityToElementMap=" << _entityToElementMap;
    }

    return findEntityByEntityItemID(entityID);
}

EntityItem* EntityTree::findEntityByEntityItemID(const EntityItemID& entityID) /*const*/ {
    EntityItem* foundEntity = NULL;
    EntityTreeElement* containingElement = getContainingElement(entityID);
    if (containingElement) {
        foundEntity = containingElement->getEntityWithEntityItemID(entityID);
    }
    return foundEntity;
}

EntityItemID EntityTree::assignEntityID(const EntityItemID& entityItemID) {
    assert(getIsServer()); // NOTE: this only operates on an server tree.
    assert(!getContainingElement(entityItemID)); // NOTE: don't call this for existing entityIDs

    // The EntityItemID is responsible for assigning actual IDs and keeping track of them.
    return entityItemID.assignActualIDForToken();
}

int EntityTree::processEditPacketData(PacketType packetType, const unsigned char* packetData, int packetLength,
                    const unsigned char* editData, int maxLength, const SharedNodePointer& senderNode) {

    //qDebug() << "EntityTree::processEditPacketData().... ******************";

    assert(getIsServer()); // NOTE: this only operates on an server tree.

    int processedBytes = 0;
    // we handle these types of "edit" packets
    switch (packetType) {
        case PacketTypeEntityAddOrEdit: {
            //qDebug() << "EntityTree::processEditPacketData()....";

            EntityItemID entityItemID;
            EntityItemProperties properties;
            
            bool validEditPacket = EntityItemProperties::decodeEntityEditPacket(editData, maxLength,
                                                    processedBytes, entityItemID, properties);
            
            // If we got a valid edit packet, then it could be a new entity or it could be an update to
            // an existing entity... handle appropriately
            if (validEditPacket) {
            
                // If this is a knownID, then it should exist in our tree
                if (entityItemID.isKnownID) {
                    // search for the entity by EntityItemID
                    EntityItem* existingEntity = findEntityByEntityItemID(entityItemID);
                    
                    // if the entityItem exists, then update it
                    if (existingEntity) {
                        updateEntity(entityItemID, properties);
                    } else {
                        qDebug() << "User attempted to edit an unknown entity.";
                    }
                } else {


//
// NOTE: We need to fix this... we can't have the creator tokens in the server side map... because if we do that
// then we will have multiple creator tokens with the same id from different editors... since assignEntityID() 
// checks for the ID already existing in the map, this will assert/abort.
//
// But we do want the creator tokens in the client side version of the map... 
// so maybe the fix is just to ....


                    // this is a new entity... assign a new entityID
qDebug() << "EntityTree::processEditPacketData() ... BEFORE assignEntityID()... entityItemID=" << entityItemID;
                    entityItemID = assignEntityID(entityItemID);
qDebug() << "EntityTree::processEditPacketData() ... AFTER assignEntityID()... entityItemID=" << entityItemID;
                    
                    EntityItem* newEntity = addEntity(entityItemID, properties);
                    if (newEntity) {
                        notifyNewlyCreatedEntity(*newEntity, senderNode);
                    }
                }
            }
            break;
        }

        default:
            processedBytes = 0;
            break;
    }
    
    return processedBytes;
}

void EntityTree::notifyNewlyCreatedEntity(const EntityItem& newEntity, const SharedNodePointer& senderNode) {
    _newlyCreatedHooksLock.lockForRead();
    for (size_t i = 0; i < _newlyCreatedHooks.size(); i++) {
        _newlyCreatedHooks[i]->entityCreated(newEntity, senderNode);
    }
    _newlyCreatedHooksLock.unlock();
}

void EntityTree::addNewlyCreatedHook(NewlyCreatedEntityHook* hook) {
    _newlyCreatedHooksLock.lockForWrite();
    _newlyCreatedHooks.push_back(hook);
    _newlyCreatedHooksLock.unlock();
}

void EntityTree::removeNewlyCreatedHook(NewlyCreatedEntityHook* hook) {
    _newlyCreatedHooksLock.lockForWrite();
    for (size_t i = 0; i < _newlyCreatedHooks.size(); i++) {
        if (_newlyCreatedHooks[i] == hook) {
            _newlyCreatedHooks.erase(_newlyCreatedHooks.begin() + i);
            break;
        }
    }
    _newlyCreatedHooksLock.unlock();
}


bool EntityTree::updateOperation(OctreeElement* element, void* extraData) {
    EntityTreeUpdateArgs* args = static_cast<EntityTreeUpdateArgs*>(extraData);
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
    entityTreeElement->update(*args);
    return true;
}

bool EntityTree::pruneOperation(OctreeElement* element, void* extraData) {
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        EntityTreeElement* childAt = entityTreeElement->getChildAtIndex(i);

/*
if (childAt) {
    qDebug() << "consider pruning child" << i 
             << "childAt=" << childAt
             << "isLeaf=" << (childAt ? childAt->isLeaf() : false)
             << "hasEntities=" << (childAt ? childAt->hasEntities() : false);
}
*/

        if (childAt && childAt->isLeaf() && !childAt->hasEntities()) {
//qDebug() << "pruning child" << i;        
            entityTreeElement->deleteChildAtIndex(i);
        }
    }
    return true;
}

void EntityTree::changeEntityState(EntityItem* const entity, EntityItem::SimuationState oldState, EntityItem::SimuationState newState) {

    bool wantDebug = false;
    
    if (wantDebug) {
        qDebug() << "EntityTree::changeEntityState()....";
        qDebug() << "     oldState:" << oldState;
        qDebug() << "     newState:" << newState;

        qDebug() << "EntityTree::changeEntityState() BEFORE....";
        qDebug() << "     _changingEntities:" << _changingEntities;
        qDebug() << "     _movingEntities:" << _movingEntities;
    }

    //if (oldState != newState) {
        switch (oldState) {
            case EntityItem::Changing:
                _changingEntities.removeAll(entity);
                break;

            case EntityItem::Moving:
                _movingEntities.removeAll(entity);
                break;
            default:
                break;
        }


        switch (newState) {
            case EntityItem::Changing:
                _changingEntities.push_back(entity);
                break;

            case EntityItem::Moving:
                _movingEntities.push_back(entity);
                break;
            default:
                break;
        }
    //}


    if (wantDebug) {
        qDebug() << "EntityTree::changeEntityState() AFTER....";
        qDebug() << "     _changingEntities:" << _changingEntities;
        qDebug() << "     _movingEntities:" << _movingEntities;
    }

}

void EntityTree::update() {

    bool wantDebug = false;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NEW CODE!!!!
//
// our new strategy should be to segregate entities into three classes:
//   1) stationary things that are not changing - most models
//   2) stationary things that are animating - they can be touched linearly and they don't change the tree
//   3) moving things - these need to scan the tree and update accordingly

    lockForWrite();
    quint64 now = usecTimestampNow();

    //_movingEntities; // entities that are moving as part of update
    //_changingEntities; // entities that are changing (like animating), but not moving
    
    
    for (int i = 0; i < _changingEntities.size(); i++) {
        EntityItem* thisEntity = _changingEntities[i];
        thisEntity->update(now);
    }
    
    MovingEntitiesOperator moveOperator(this);
    
    QSet<EntityItem*> entitiesBecomingStatic;
    QSet<EntityItem*> entitiesBecomingChanging;

    for (int i = 0; i < _movingEntities.size(); i++) {
        EntityItem* thisEntity = _movingEntities[i];
        AACube oldCube = thisEntity->getAACube();
        thisEntity->update(now);
        AACube newCube = thisEntity->getAACube();

        if (wantDebug) {
            qDebug() << "EntityTree::update() thisEntity=" << thisEntity;
            qDebug() << "   oldCube=" << oldCube;
            qDebug() << "   newCube=" << newCube;
        }
                
        moveOperator.addEntityToMoveList(thisEntity, oldCube, newCube);

        // check to see if this entity is no longer moving
        EntityItem::SimuationState newState = thisEntity->getSimulationState();
        if (newState == EntityItem::Changing) {
            entitiesBecomingChanging << thisEntity;
        } else if (newState == EntityItem::Static) {
            entitiesBecomingStatic << thisEntity;
        }
    }
    recurseTreeWithOperator(&moveOperator);

    // for any and all entities that were moving but are now either static or changing
    // change their state accordingly
    foreach(EntityItem* entity, entitiesBecomingStatic) {
        changeEntityState(entity, EntityItem::Moving, EntityItem::Static);
    }
    foreach(EntityItem* entity, entitiesBecomingChanging) {
        changeEntityState(entity, EntityItem::Moving, EntityItem::Changing);
    }

    unlock();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// OLD CODE!!!!
//
// The old update code scanned the entire tree... this is very expensive, since not all entities are getting simulated
//
//    // XXXBHG: replace storeEntity with new API!!
//    //qDebug() << "EntityTree::update().... NOT YET IMPLEMENTED!!!";
//    #if 0 //////////////////////////////////////////////////////
//
//    lockForWrite();
//    _isDirty = true;
//
//    // TODO: could we manage this by iterating the known entities map/hash? Would that be faster?
//    EntityTreeUpdateArgs args;
//    recurseTreeWithOperation(updateOperation, &args);
//
//    // now add back any of the particles that moved elements....
//    int movingEntities = args._movingEntities.size();
//    
//    for (int i = 0; i < movingEntities; i++) {
//    
//        bool shouldDie = args._movingEntities[i]->getShouldBeDeleted();
//
//        // if the particle is still inside our total bounds, then re-add it
//        AACube treeBounds = getRoot()->getAACube();
//
//        if (!shouldDie && treeBounds.contains(args._movingEntities[i]->getPosition())) {
//            storeEntity(*args._movingEntities[i]);
//        } else {
//            uint32_t entityItemID = args._movingEntities[i]->getID();
//            quint64 deletedAt = usecTimestampNow();
//            _recentlyDeletedEntitiesLock.lockForWrite();
//            _recentlyDeletedEntityItemIDs.insert(deletedAt, entityItemID);
//            _recentlyDeletedEntitiesLock.unlock();
//        }
//    }
//
//
//    #endif // 0 //////////////////////////////////////////////////////
//
//
//    // prune the tree...
//    /*
//    lockForWrite();
//qDebug() << "pruning tree";
//    recurseTreeWithOperation(pruneOperation, NULL);
//    unlock();
//    */
}


bool EntityTree::hasEntitiesDeletedSince(quint64 sinceTime) {
    // we can probably leverage the ordered nature of QMultiMap to do this quickly...
    bool hasSomethingNewer = false;

    _recentlyDeletedEntitiesLock.lockForRead();
    QMultiMap<quint64, uint32_t>::const_iterator iterator = _recentlyDeletedEntityItemIDs.constBegin();
    while (iterator != _recentlyDeletedEntityItemIDs.constEnd()) {
        //qDebug() << "considering... time/key:" << iterator.key();
        if (iterator.key() > sinceTime) {
            //qDebug() << "YES newer... time/key:" << iterator.key();
            hasSomethingNewer = true;
        }
        ++iterator;
    }
    _recentlyDeletedEntitiesLock.unlock();
    return hasSomethingNewer;
}

// sinceTime is an in/out parameter - it will be side effected with the last time sent out
bool EntityTree::encodeEntitiesDeletedSince(OCTREE_PACKET_SEQUENCE sequenceNumber, quint64& sinceTime, unsigned char* outputBuffer,
                                            size_t maxLength, size_t& outputLength) {

qDebug() << "EntityTree::encodeEntitiesDeletedSince()";

    bool hasMoreToSend = true;

    unsigned char* copyAt = outputBuffer;
    size_t numBytesPacketHeader = populatePacketHeader(reinterpret_cast<char*>(outputBuffer), PacketTypeEntityErase);
    copyAt += numBytesPacketHeader;
    outputLength = numBytesPacketHeader;

    // pack in flags
    OCTREE_PACKET_FLAGS flags = 0;
    OCTREE_PACKET_FLAGS* flagsAt = (OCTREE_PACKET_FLAGS*)copyAt;
    *flagsAt = flags;
    copyAt += sizeof(OCTREE_PACKET_FLAGS);
    outputLength += sizeof(OCTREE_PACKET_FLAGS);

    // pack in sequence number
    OCTREE_PACKET_SEQUENCE* sequenceAt = (OCTREE_PACKET_SEQUENCE*)copyAt;
    *sequenceAt = sequenceNumber;
    copyAt += sizeof(OCTREE_PACKET_SEQUENCE);
    outputLength += sizeof(OCTREE_PACKET_SEQUENCE);

    // pack in timestamp
    OCTREE_PACKET_SENT_TIME now = usecTimestampNow();
    OCTREE_PACKET_SENT_TIME* timeAt = (OCTREE_PACKET_SENT_TIME*)copyAt;
    *timeAt = now;
    copyAt += sizeof(OCTREE_PACKET_SENT_TIME);
    outputLength += sizeof(OCTREE_PACKET_SENT_TIME);

    uint16_t numberOfIds = 0; // placeholder for now
    unsigned char* numberOfIDsAt = copyAt;
    memcpy(copyAt, &numberOfIds, sizeof(numberOfIds));
    copyAt += sizeof(numberOfIds);
    outputLength += sizeof(numberOfIds);
    
    // we keep a multi map of entity IDs to timestamps, we only want to include the entity IDs that have been
    // deleted since we last sent to this node
    _recentlyDeletedEntitiesLock.lockForRead();
    QMultiMap<quint64, uint32_t>::const_iterator iterator = _recentlyDeletedEntityItemIDs.constBegin();
    while (iterator != _recentlyDeletedEntityItemIDs.constEnd()) {
        QList<uint32_t> values = _recentlyDeletedEntityItemIDs.values(iterator.key());
        for (int valueItem = 0; valueItem < values.size(); ++valueItem) {

            // if the timestamp is more recent then out last sent time, include it
            if (iterator.key() > sinceTime) {
                uint32_t entityID = values.at(valueItem);
                memcpy(copyAt, &entityID, sizeof(entityID));
                copyAt += sizeof(entityID);
                outputLength += sizeof(entityID);
                numberOfIds++;

                // check to make sure we have room for one more id...
                if (outputLength + sizeof(uint32_t) > maxLength) {
                    break;
                }
            }
        }

        // check to make sure we have room for one more id...
        if (outputLength + sizeof(uint32_t) > maxLength) {

            // let our caller know how far we got
            sinceTime = iterator.key();
            break;
        }
        ++iterator;
    }

    // if we got to the end, then we're done sending
    if (iterator == _recentlyDeletedEntityItemIDs.constEnd()) {
        hasMoreToSend = false;
    }
    _recentlyDeletedEntitiesLock.unlock();

    // replace the correct count for ids included
    memcpy(numberOfIDsAt, &numberOfIds, sizeof(numberOfIds));
    return hasMoreToSend;
}

// called by the server when it knows all nodes have been sent deleted packets

void EntityTree::forgetEntitiesDeletedBefore(quint64 sinceTime) {
    //qDebug() << "forgetEntitiesDeletedBefore()";
    QSet<quint64> keysToRemove;

    _recentlyDeletedEntitiesLock.lockForWrite();
    QMultiMap<quint64, uint32_t>::iterator iterator = _recentlyDeletedEntityItemIDs.begin();

    // First find all the keys in the map that are older and need to be deleted
    while (iterator != _recentlyDeletedEntityItemIDs.end()) {
        if (iterator.key() <= sinceTime) {
            keysToRemove << iterator.key();
        }
        ++iterator;
    }

    // Now run through the keysToRemove and remove them
    foreach (quint64 value, keysToRemove) {
        //qDebug() << "removing the key, _recentlyDeletedEntityItemIDs.remove(value); time/key:" << value;
        _recentlyDeletedEntityItemIDs.remove(value);
    }
    
    _recentlyDeletedEntitiesLock.unlock();
}


void EntityTree::processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {
    
#if 0 ///////////////


    qDebug() << "EntityTree::processEraseMessage()...";

    const unsigned char* packetData = (const unsigned char*)dataByteArray.constData();
    const unsigned char* dataAt = packetData;
    size_t packetLength = dataByteArray.size();

    size_t numBytesPacketHeader = numBytesForPacketHeader(dataByteArray);
    size_t processedBytes = numBytesPacketHeader;
    dataAt += numBytesPacketHeader;

    dataAt += sizeof(OCTREE_PACKET_FLAGS);
    dataAt += sizeof(OCTREE_PACKET_SEQUENCE);
    dataAt += sizeof(OCTREE_PACKET_SENT_TIME);

    uint16_t numberOfIds = 0; // placeholder for now
    memcpy(&numberOfIds, dataAt, sizeof(numberOfIds));
    dataAt += sizeof(numberOfIds);
    processedBytes += sizeof(numberOfIds);

    if (numberOfIds > 0) {
        QSet<EntityItemID> entityItemIDsToDelete;

        for (size_t i = 0; i < numberOfIds; i++) {
            if (processedBytes + sizeof(uint32_t) > packetLength) {
                break; // bail to prevent buffer overflow
            }

            uint32_t entityID = 0; // placeholder for now
            memcpy(&entityID, dataAt, sizeof(entityID));
            dataAt += sizeof(entityID);
            processedBytes += sizeof(entityID);
            
            EntityItemID entityItemID(entityID);
            entityItemIDsToDelete << entityItemID;
            qDebug() << "EntityTree::processEraseMessage()... entityItemIDsToDelete << entityItemID=" << entityItemID;
        }
        qDebug() << "EntityTree::processEraseMessage()... deleteEntities(entityItemIDsToDelete)";
        deleteEntities(entityItemIDsToDelete);
    }
    
#endif // 0 ///////////////
    
}


EntityTreeElement* EntityTree::getContainingElement(const EntityItemID& entityItemID)  /*const*/ {
    //qDebug() << "_entityToElementMap=" << _entityToElementMap;
    
    bool wantDebug = false;
    
    if (wantDebug) {
        qDebug() << "EntityTree::getContainingElement() entityItemID=" << entityItemID;
        debugDumpMap();
    }

    // TODO: do we need to make this thread safe? Or is it acceptable as is
    if (_entityToElementMap.contains(entityItemID)) {
        return _entityToElementMap.value(entityItemID);
    } else if (entityItemID.creatorTokenID != UNKNOWN_ENTITY_TOKEN){
        // check the creator token version too...

        if (wantDebug) {
            qDebug() << "EntityTree::getContainingElement() checking the creator token...";
        }
        
        EntityItemID creatorTokenOnly;
        creatorTokenOnly.id = UNKNOWN_ENTITY_ID;
        creatorTokenOnly.creatorTokenID = entityItemID.creatorTokenID;
        creatorTokenOnly.isKnownID = false;

        if (_entityToElementMap.contains(entityItemID)) {
            if (wantDebug) {
                qDebug() << "EntityTree::getContainingElement() found as creator token...";
            }
            return _entityToElementMap.value(entityItemID);
        }

    }
    return NULL;
}

// TODO: do we need to make this thread safe? Or is it acceptable as is
void EntityTree::resetContainingElement(const EntityItemID& entityItemID, EntityTreeElement* element) {
    assert(entityItemID.id != UNKNOWN_ENTITY_ID);
    assert(entityItemID.creatorTokenID != UNKNOWN_ENTITY_TOKEN);
    assert(element);
    
    // remove the old version with the creatorTokenID
    EntityItemID creatorTokenVersion;
    creatorTokenVersion.id = UNKNOWN_ENTITY_ID;
    creatorTokenVersion.isKnownID = false;
    creatorTokenVersion.creatorTokenID = entityItemID.creatorTokenID;
    _entityToElementMap.remove(creatorTokenVersion);

    // set the new version with both creator token and real ID
    _entityToElementMap[entityItemID] = element;
}

void EntityTree::setContainingElement(const EntityItemID& entityItemID, EntityTreeElement* element) {
    // TODO: do we need to make this thread safe? Or is it acceptable as is
    
    // If we're a sever side tree, we always remove the creator tokens from our map items
    EntityItemID storedEntityItemID = entityItemID;
    
    if (getIsServer()) {
        storedEntityItemID.creatorTokenID = UNKNOWN_ENTITY_TOKEN;
    }
    
    if (element) {
        _entityToElementMap[storedEntityItemID] = element;
    } else {
        _entityToElementMap.remove(storedEntityItemID);
    }

    bool wantDebug = false;

    if (wantDebug) {
        qDebug() << "setContainingElement() entityItemID=" << entityItemID 
                << "storedEntityItemID=" << storedEntityItemID << "element=" << element;
        debugDumpMap();
        //qDebug() << "AFTER _entityToElementMap=" << _entityToElementMap;
    }
}

void EntityTree::debugDumpMap() {
    qDebug() << "EntityTree::debugDumpMap() --------------------------";
    QHashIterator<EntityItemID, EntityTreeElement*> i(_entityToElementMap);
    while (i.hasNext()) {
        i.next();
        qDebug() << i.key() << ": " << i.value();
    }
    qDebug() << "-----------------------------------------------------";
}

class DebugOperator : public RecurseOctreeOperator {
public:
    virtual bool PreRecursion(OctreeElement* element);
    virtual bool PostRecursion(OctreeElement* element) { return true; };
    virtual OctreeElement* PossiblyCreateChildAt(OctreeElement* element, int childIndex) { return NULL; }
};

bool DebugOperator::PreRecursion(OctreeElement* element) {
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);
    entityTreeElement->debugDump();
    return true;
}

void EntityTree::dumpTree() {
    // First, look for the existing entity in the tree..
    DebugOperator theOperator;
    recurseTreeWithOperator(&theOperator);
}

void EntityTree::sendEntities(EntityEditPacketSender* packetSender, float x, float y, float z) {
    SendEntitiesOperationArgs args;
    args.packetSender = packetSender;
    args.root = glm::vec3(x, y, z);
    recurseTreeWithOperation(sendEntitiesOperation, &args);
    packetSender->releaseQueuedMessages();
}

bool EntityTree::sendEntitiesOperation(OctreeElement* element, void* extraData) {
    SendEntitiesOperationArgs* args = static_cast<SendEntitiesOperationArgs*>(extraData);
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);

    // TODO: implement this!!
    /**
    const QList<ModelItem>& modelList = modelTreeElement->getModels();

    for (int i = 0; i < modelList.size(); i++) {
        uint32_t creatorTokenID = ModelItem::getNextCreatorTokenID();
        ModelItemID id(NEW_MODEL, creatorTokenID, false);
        ModelItemProperties properties;
        properties.copyFromNewModelItem(modelList.at(i));
        properties.setPosition(properties.getPosition() + args->root);
        args->packetSender->queueModelEditMessage(PacketTypeModelAddOrEdit, id, properties);
    }
    **/

    return true;
}

