//
//  EntityTree.cpp
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityTree.h"

EntityTree::EntityTree(bool shouldReaverage) : Octree(shouldReaverage) {
    _rootElement = createNewElement();
}

EntityTreeElement* EntityTree::createNewElement(unsigned char * octalCode) {
    EntityTreeElement* newElement = new EntityTreeElement(octalCode);
    newElement->setTree(this);
    return newElement;
}

void EntityTree::eraseAllOctreeElements() {
    _modelToElementMap.clear();
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

class StoreEntityOperator : public RecurseOctreeOperator {
public:
    StoreEntityOperator(EntityTree* tree, const EntityItem& searchEntity);
    virtual bool PreRecursion(OctreeElement* element);
    virtual bool PostRecursion(OctreeElement* element);
    virtual OctreeElement* PossiblyCreateChildAt(OctreeElement* element, int childIndex);
private:
    EntityTree* _tree;
    const EntityItem& _newEntity;
    const EntityItem* _oldEntity;
    EntityTreeElement* _containingElement;
    bool _foundOld;
    bool _foundNew;
    quint64 _changeTime;

    bool subTreeContainsOldEntity(OctreeElement* element);
    bool subTreeContainsNewEntity(OctreeElement* element);
};

StoreEntityOperator::StoreEntityOperator(EntityTree* tree, const EntityItem& searchEntity) :
    _tree(tree),
    _newEntity(searchEntity),
    _oldEntity(NULL),
    _containingElement(NULL),
    _foundOld(false),
    _foundNew(false),
    _changeTime(usecTimestampNow())
{
    // check our tree, to determine if this model is known
    _containingElement = _tree->getContainingElement(searchEntity.getEntityItemID());
    if (_containingElement) {
        _oldEntity = _containingElement->getEntityWithEntityItemID(searchEntity.getEntityItemID());
        if (!_oldEntity) {
            //assert(_oldEntity);
            qDebug() << "that's UNEXPECTED, we got a _containingElement, but couldn't find the oldEntity!";
        }
        
        // If this containing element would be the best fit for our new model, then just do the new
        // portion of the store pass, since the change path will be the same for both parts of the update
        if (_containingElement->bestFitEntityBounds(_newEntity)) {
            _foundOld = true;
        }
    } else {
        // if the old model is not known, then we can consider if found, and
        // we'll only be searching for the new location
        _foundOld = true;
    }
}

// does this model tree element contain the old model
bool StoreEntityOperator::subTreeContainsOldEntity(OctreeElement* element) {
    bool containsEntity = false;

    // If we don't have an old model, then we don't contain the model, otherwise
    // check the bounds
    if (_oldEntity) {
        AACube elementCube = element->getAACube();
        AACube modelCube = _oldEntity->getAACube();
        containsEntity = elementCube.contains(modelCube);
    }
    return containsEntity;
}

bool StoreEntityOperator::subTreeContainsNewEntity(OctreeElement* element) {
    AACube elementCube = element->getAACube();
    AACube modelCube = _newEntity.getAACube();
    return elementCube.contains(modelCube);
}


bool StoreEntityOperator::PreRecursion(OctreeElement* element) {
    EntityTreeElement* modelTreeElement = static_cast<EntityTreeElement*>(element);
    
    // In Pre-recursion, we're generally deciding whether or not we want to recurse this
    // path of the tree. For this operation, we want to recurse the branch of the tree if
    // and of the following are true:
    //   * We have not yet found the old model, and this branch contains our old model
    //   * We have not yet found the new model, and this branch contains our new model
    //
    // Note: it's often the case that the branch in question contains both the old model
    // and the new model.
    
    bool keepSearching = false; // assume we don't need to search any more
    
    // If we haven't yet found the old model, and this subTreeContains our old
    // model, then we need to keep searching.
    if (!_foundOld && subTreeContainsOldEntity(element)) {
        
        // If this is the element we're looking for, then ask it to remove the old model
        // and we can stop searching.
        if (modelTreeElement == _containingElement) {
        
            // If the containgElement IS NOT the best fit for the new model properties
            // then we need to remove it, and the updateEntity below will store it in the
            // correct element.
            if (!_containingElement->bestFitEntityBounds(_newEntity)) {
                modelTreeElement->removeEntityWithEntityItemID(_newEntity.getEntityItemID());
                
                // If we haven't yet found the new location, then we need to 
                // make sure to remove our model to element map, because for
                // now we're not in that map
                if (!_foundNew) {
                    _tree->setContainingElement(_newEntity.getEntityItemID(), NULL);
                }
            }
            _foundOld = true;
        } else {
            // if this isn't the element we're looking for, then keep searching
            keepSearching = true;
        }
    }

    // If we haven't yet found the new model,  and this subTreeContains our new
    // model, then we need to keep searching.
    if (!_foundNew && subTreeContainsNewEntity(element)) {
    
        // Note: updateEntity() will only operate on correctly found models and/or add them
        // to the element if they SHOULD be stored there.
        if (modelTreeElement->updateEntity(_newEntity)) {
            //qDebug() << "StoreEntityOperator::PreRecursion()... model was updated!";
            _foundNew = true;
            // NOTE: don't change the keepSearching here, if it came in here
            // false then we stay false, if it came in here true, then it
            // means we're still searching for our old model and this branch
            // contains our old model. In which case we want to keep searching.
            
        } else {
            keepSearching = true;
        }
    }
    
    return keepSearching; // if we haven't yet found it, keep looking
}

bool StoreEntityOperator::PostRecursion(OctreeElement* element) {
    // Post-recursion is the unwinding process. For this operation, while we
    // unwind we want to mark the path as being dirty if we changed it below.
    // We might have two paths, one for the old model and one for the new model.
    bool keepSearching = !_foundOld || !_foundNew;

    // As we unwind, if we're in either of these two paths, we mark our element
    // as dirty.
    if ((_foundOld && subTreeContainsOldEntity(element)) ||
            (_foundNew && subTreeContainsNewEntity(element))) {
        element->markWithChangedTime();
    }
    return keepSearching; // if we haven't yet found it, keep looking
}

OctreeElement* StoreEntityOperator::PossiblyCreateChildAt(OctreeElement* element, int childIndex) { 
    // If we're getting called, it's because there was no child element at this index while recursing.
    // We only care if this happens while still searching for the new model location.
    // Check to see if 
    if (!_foundNew) {
        int indexOfChildContainingNewEntity = element->getMyChildContaining(_newEntity.getAACube());
        
        if (childIndex == indexOfChildContainingNewEntity) {
            return element->addChildAtIndex(childIndex);
        }
    }
    return NULL; 
}


void EntityTree::storeEntity(const EntityItem& model, const SharedNodePointer& senderNode) {
    // NOTE: callers must lock the tree before using this method

    // First, look for the existing model in the tree..
    StoreEntityOperator theOperator(this, model);

    recurseTreeWithOperator(&theOperator);
    _isDirty = true;

    bool wantDebug = false;
    if (wantDebug) {
        EntityTreeElement* containingElement = getContainingElement(model.getEntityItemID());
        qDebug() << "EntityTree::storeEntity().... after store... containingElement=" << containingElement;
    }

}

void EntityTree::updateEntity(const EntityItemID& modelID, const EntityItemProperties& properties) {
    EntityItem updateItem(modelID);

    bool wantDebug = false;
    if (wantDebug) {    
        qDebug() << "EntityTree::updateEntity(modelID, properties) line:" << __LINE__ << "updateItem:";
        updateItem.debugDump();
    }
    
    // since the properties might not be complete, they may only contain some values,
    // we need to first see if we already have the model in our tree, and make a copy of
    // its existing properties first
    EntityTreeElement* containingElement = getContainingElement(modelID);

    if (wantDebug) {    
        qDebug() << "EntityTree::updateEntity(modelID, properties) containingElement=" << containingElement;
    }

    if (containingElement) {
        const EntityItem* oldEntity = containingElement->getEntityWithEntityItemID(modelID);
        if (oldEntity) {
            EntityItemProperties oldProps = oldEntity->getProperties();
            if (wantDebug) {    
                qDebug() << "EntityTree::updateEntity(modelID, properties) ********** COPY PROPERTIES FROM oldEntity=" << oldEntity << "*******************";
                qDebug() << "EntityTree::updateEntity(modelID, properties) oldEntity=" << oldEntity;
                oldProps.debugDump();
                qDebug() << "EntityTree::updateEntity(modelID, properties) line:" << __LINE__ << "about to call updateItem.setProperties(oldProps);";
            }
            updateItem.setProperties(oldProps, true); // force copy

            if (wantDebug) {    
                qDebug() << "EntityTree::updateEntity(modelID, properties) line:" << __LINE__ << "updateItem:";
                updateItem.debugDump();
            }

        } else {
            if (wantDebug) {    
                qDebug() << "EntityTree::updateEntity(modelID, properties) WAIT WHAT!!! COULDN'T FIND oldEntity=" << oldEntity;
            }
        }
    }
    updateItem.setProperties(properties);

    if (wantDebug) {    
        qDebug() << "EntityTree::updateEntity(modelID, properties) line:" << __LINE__ << "updateItem:";
        updateItem.debugDump();
    }

    storeEntity(updateItem);
}

void EntityTree::addEntity(const EntityItemID& modelID, const EntityItemProperties& properties) {
    EntityItem updateItem(modelID, properties);
    storeEntity(updateItem);
}

class EntityToDeleteDetails {
public:
    const EntityItem* model;
    AACube cube;
    EntityTreeElement* containingElement;
};

inline uint qHash(const EntityToDeleteDetails& a, uint seed) {
    return qHash(a.model->getEntityItemID(), seed);
}

inline bool operator==(const EntityToDeleteDetails& a, const EntityToDeleteDetails& b) {
    return a.model->getEntityItemID() == b.model->getEntityItemID();
}

class DeleteEntityOperator : public RecurseOctreeOperator {
public:
    DeleteEntityOperator(EntityTree* tree);
    DeleteEntityOperator(EntityTree* tree, const EntityItemID& searchEntityID);
    void modelToDelete(const EntityItemID& searchEntityID);
    virtual bool PreRecursion(OctreeElement* element);
    virtual bool PostRecursion(OctreeElement* element);
private:
    EntityTree* _tree;
    QSet<EntityToDeleteDetails> _modelsToDelete;
    quint64 _changeTime;
    int _foundCount;
    int _lookingCount;
    bool subTreeContainsSomeEntitysToDelete(OctreeElement* element);
};

DeleteEntityOperator::DeleteEntityOperator(EntityTree* tree, const EntityItemID& searchEntityID) :
    _tree(tree),
    _changeTime(usecTimestampNow()),
    _foundCount(0),
    _lookingCount(0)
{
    modelToDelete(searchEntityID);
}

DeleteEntityOperator::DeleteEntityOperator(EntityTree* tree) :
    _tree(tree),
    _changeTime(usecTimestampNow()),
    _foundCount(0),
    _lookingCount(0)
{
}

void DeleteEntityOperator::modelToDelete(const EntityItemID& searchEntityID) {
    // check our tree, to determine if this model is known
    EntityToDeleteDetails details;
    details.containingElement = _tree->getContainingElement(searchEntityID);

    if (details.containingElement) {
        details.model = details.containingElement->getEntityWithEntityItemID(searchEntityID);
        if (!details.model) {
            //assert(false);
            qDebug() << "that's UNEXPECTED, we got a _containingElement, but couldn't find the oldEntity!";
        } else {
            details.cube = details.model->getAACube();
            _modelsToDelete << details;
            _lookingCount++;
        }
    }
}


// does this model tree element contain the old model
bool DeleteEntityOperator::subTreeContainsSomeEntitysToDelete(OctreeElement* element) {
    bool containsEntity = false;

    // If we don't have an old model, then we don't contain the model, otherwise
    // check the bounds
    if (_modelsToDelete.size() > 0) {
        AACube elementCube = element->getAACube();
        foreach(const EntityToDeleteDetails& details, _modelsToDelete) {
            if (elementCube.contains(details.cube)) {
                containsEntity = true;
                break; // if it contains at least one, we're good to go
            }
        }
    }
    return containsEntity;
}

bool DeleteEntityOperator::PreRecursion(OctreeElement* element) {
    EntityTreeElement* modelTreeElement = static_cast<EntityTreeElement*>(element);
    
    // In Pre-recursion, we're generally deciding whether or not we want to recurse this
    // path of the tree. For this operation, we want to recurse the branch of the tree if
    // and of the following are true:
    //   * We have not yet found the old model, and this branch contains our old model
    //   * We have not yet found the new model, and this branch contains our new model
    //
    // Note: it's often the case that the branch in question contains both the old model
    // and the new model.
    
    bool keepSearching = false; // assume we don't need to search any more
    
    // If we haven't yet found all the models, and this sub tree contains at least one of our
    // models, then we need to keep searching.
    if ((_foundCount < _lookingCount) && subTreeContainsSomeEntitysToDelete(element)) {

        // check against each of our search models
        foreach(const EntityToDeleteDetails& details, _modelsToDelete) {

            // If this is the element we're looking for, then ask it to remove the old model
            // and we can stop searching.
            if (modelTreeElement == details.containingElement) {

                // This is a good place to delete it!!!
                EntityItemID entityItemID = details.model->getEntityItemID();
                modelTreeElement->removeEntityWithEntityItemID(entityItemID);
                _tree->setContainingElement(entityItemID, NULL);
                _foundCount++;
            }
        }
        
        // if we haven't found all of our search for models, then keep looking
        keepSearching = (_foundCount < _lookingCount);
    }

    return keepSearching; // if we haven't yet found it, keep looking
}

bool DeleteEntityOperator::PostRecursion(OctreeElement* element) {
    // Post-recursion is the unwinding process. For this operation, while we
    // unwind we want to mark the path as being dirty if we changed it below.
    // We might have two paths, one for the old model and one for the new model.
    bool keepSearching = (_foundCount < _lookingCount);

    // As we unwind, if we're in either of these two paths, we mark our element
    // as dirty.
    if ((subTreeContainsSomeEntitysToDelete(element))) {
        element->markWithChangedTime();
    }
    return keepSearching; // if we haven't yet found it, keep looking
}

void EntityTree::deleteEntity(const EntityItemID& modelID) {
    // NOTE: callers must lock the tree before using this method

    // First, look for the existing model in the tree..
    DeleteEntityOperator theOperator(this, modelID);

    recurseTreeWithOperator(&theOperator);
    _isDirty = true;

    bool wantDebug = false;
    if (wantDebug) {
        EntityTreeElement* containingElement = getContainingElement(modelID);
        qDebug() << "EntityTree::storeEntity().... after store... containingElement=" << containingElement;
    }
}

void EntityTree::deleteEntitys(QSet<EntityItemID> modelIDs) {
    // NOTE: callers must lock the tree before using this method

    DeleteEntityOperator theOperator(this);
    foreach(const EntityItemID& modelID, modelIDs) {
        // First, look for the existing model in the tree..
        theOperator.modelToDelete(modelID);
    }

    recurseTreeWithOperator(&theOperator);
    _isDirty = true;

    bool wantDebug = false;
    if (wantDebug) {
        foreach(const EntityItemID& modelID, modelIDs) {
            EntityTreeElement* containingElement = getContainingElement(modelID);
            qDebug() << "EntityTree::storeEntity().... after store... containingElement=" << containingElement;
        }
    }
}

// scans the tree and handles mapping locally created models to know IDs.
// in the event that this tree is also viewing the scene, then we need to also
// search the tree to make sure we don't have a duplicate model from the viewing
// operation.
bool EntityTree::findAndUpdateEntityItemIDOperation(OctreeElement* element, void* extraData) {
    bool keepSearching = true;

    FindAndUpdateEntityItemIDArgs* args = static_cast<FindAndUpdateEntityItemIDArgs*>(extraData);
    EntityTreeElement* modelTreeElement = static_cast<EntityTreeElement*>(element);

    // Note: updateEntityItemID() will only operate on correctly found models
    modelTreeElement->updateEntityItemID(args);

    // if we've found and replaced both the creatorTokenID and the viewedEntity, then we
    // can stop looking, otherwise we will keep looking    
    if (args->creatorTokenFound && args->viewedEntityFound) {
        keepSearching = false;
    }
    
    return keepSearching;
}

void EntityTree::handleAddEntityResponse(const QByteArray& packet) {
    const bool wantDebug = false;

    if (wantDebug) {
        qDebug() << "EntityTree::handleAddEntityResponse()..."; 
    }

    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    
    const unsigned char* dataAt = reinterpret_cast<const unsigned char*>(packet.data()) + numBytesPacketHeader;

    uint32_t creatorTokenID;
    memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
    dataAt += sizeof(creatorTokenID);

    uint32_t modelID;
    memcpy(&modelID, dataAt, sizeof(modelID));
    dataAt += sizeof(modelID);

    if (wantDebug) {
        qDebug() << "    creatorTokenID=" << creatorTokenID;
        qDebug() << "    modelID=" << modelID;
    }

    // update models in our tree
    bool assumeEntityFound = !getIsViewing(); // if we're not a viewing tree, then we don't have to find the actual model
    FindAndUpdateEntityItemIDArgs args = { 
        modelID, 
        creatorTokenID, 
        false, 
        assumeEntityFound,
        getIsViewing() 
    };
    
    if (wantDebug) {
        qDebug() << "looking for creatorTokenID=" << creatorTokenID << " modelID=" << modelID 
                << " getIsViewing()=" << getIsViewing();
    }
    lockForWrite();
    // TODO: Switch this to use list of known model IDs....
    recurseTreeWithOperation(findAndUpdateEntityItemIDOperation, &args);
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
    EntityTreeElement* modelTreeElement = static_cast<EntityTreeElement*>(element);

    glm::vec3 penetration;
    bool sphereIntersection = modelTreeElement->getAACube().findSpherePenetration(args->position,
                                                                    args->targetRadius, penetration);

    // If this modelTreeElement contains the point, then search it...
    if (sphereIntersection) {
        const EntityItem* thisClosestEntity = modelTreeElement->getClosestEntity(args->position);

        // we may have gotten NULL back, meaning no model was available
        if (thisClosestEntity) {
            glm::vec3 modelPosition = thisClosestEntity->getPosition();
            float distanceFromPointToEntity = glm::distance(modelPosition, args->position);

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
        return true; // keep searching in case children have closer models
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
    QVector<const EntityItem*> models;
};


bool EntityTree::findInSphereOperation(OctreeElement* element, void* extraData) {
    FindAllNearPointArgs* args = static_cast<FindAllNearPointArgs*>(extraData);
    glm::vec3 penetration;
    bool sphereIntersection = element->getAACube().findSpherePenetration(args->position,
                                                                    args->targetRadius, penetration);

    // If this element contains the point, then search it...
    if (sphereIntersection) {
        EntityTreeElement* modelTreeElement = static_cast<EntityTreeElement*>(element);
        modelTreeElement->getEntities(args->position, args->targetRadius, args->models);
        return true; // keep searching in case children have closer models
    }

    // if this element doesn't contain the point, then none of it's children can contain the point, so stop searching
    return false;
}

void EntityTree::findEntities(const glm::vec3& center, float radius, QVector<const EntityItem*>& foundEntitys) {
    FindAllNearPointArgs args = { center, radius };
    lockForRead();
    // NOTE: This should use recursion, since this is a spatial operation
    recurseTreeWithOperation(findInSphereOperation, &args);
    unlock();
    // swap the two lists of model pointers instead of copy
    foundEntitys.swap(args.models);
}

class FindEntitysInCubeArgs {
public:
    FindEntitysInCubeArgs(const AACube& cube) 
        : _cube(cube), _foundEntitys() {
    }

    AACube _cube;
    QVector<EntityItem*> _foundEntitys;
};

bool EntityTree::findInCubeOperation(OctreeElement* element, void* extraData) {
    FindEntitysInCubeArgs* args = static_cast< FindEntitysInCubeArgs*>(extraData);
    const AACube& elementCube = element->getAACube();
    if (elementCube.touches(args->_cube)) {
        EntityTreeElement* modelTreeElement = static_cast<EntityTreeElement*>(element);
        modelTreeElement->getEntities(args->_cube, args->_foundEntitys);
        return true;
    }
    return false;
}

void EntityTree::findEntities(const AACube& cube, QVector<EntityItem*> foundEntitys) {
    FindEntitysInCubeArgs args(cube);
    lockForRead();
    // NOTE: This should use recursion, since this is a spatial operation
    recurseTreeWithOperation(findInCubeOperation, &args);
    unlock();
    // swap the two lists of model pointers instead of copy
    foundEntitys.swap(args._foundEntitys);
}

const EntityItem* EntityTree::findEntityByID(uint32_t id, bool alreadyLocked) const {
    EntityItemID modelID(id);

    bool wantDebug = false;
    if (wantDebug) {
        qDebug() << "EntityTree::findEntityByID()...";
        qDebug() << "    id=" << id;
        qDebug() << "    modelID=" << modelID;
        qDebug() << "_modelToElementMap=" << _modelToElementMap;
    }

    return findEntityByEntityItemID(modelID);
}

const EntityItem* EntityTree::findEntityByEntityItemID(const EntityItemID& modelID) const {
    const EntityItem* foundEntity = NULL;
    EntityTreeElement* containingElement = getContainingElement(modelID);
    if (containingElement) {
        foundEntity = containingElement->getEntityWithEntityItemID(modelID);
    }
    return foundEntity;
}


int EntityTree::processEditPacketData(PacketType packetType, const unsigned char* packetData, int packetLength,
                    const unsigned char* editData, int maxLength, const SharedNodePointer& senderNode) {

    int processedBytes = 0;
    // we handle these types of "edit" packets
    switch (packetType) {
        case PacketTypeEntityAddOrEdit: {
            bool isValid;
            EntityItem newEntity = EntityItem::fromEditPacket(editData, maxLength, processedBytes, this, isValid);
            if (isValid) {
                storeEntity(newEntity, senderNode);
                if (newEntity.isNewlyCreated()) {
                    notifyNewlyCreatedEntity(newEntity, senderNode);
                }
            }
        } break;

        default:
            processedBytes = 0;
            break;
    }
    
    return processedBytes;
}

void EntityTree::notifyNewlyCreatedEntity(const EntityItem& newEntity, const SharedNodePointer& senderNode) {
    _newlyCreatedHooksLock.lockForRead();
    for (size_t i = 0; i < _newlyCreatedHooks.size(); i++) {
        _newlyCreatedHooks[i]->modelCreated(newEntity, senderNode);
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
    EntityTreeElement* modelTreeElement = static_cast<EntityTreeElement*>(element);
    modelTreeElement->update(*args);
    return true;
}

bool EntityTree::pruneOperation(OctreeElement* element, void* extraData) {
    EntityTreeElement* modelTreeElement = static_cast<EntityTreeElement*>(element);
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        EntityTreeElement* childAt = modelTreeElement->getChildAtIndex(i);
        if (childAt && childAt->isLeaf() && !childAt->hasEntities()) {
            modelTreeElement->deleteChildAtIndex(i);
        }
    }
    return true;
}

void EntityTree::update() {
    lockForWrite();
    _isDirty = true;

    // TODO: could we manage this by iterating the known models map/hash? Would that be faster?
    EntityTreeUpdateArgs args;
    recurseTreeWithOperation(updateOperation, &args);

    // now add back any of the particles that moved elements....
    int movingEntitys = args._movingEntities.size();
    
    for (int i = 0; i < movingEntitys; i++) {
        bool shouldDie = args._movingEntities[i].getShouldBeDeleted();

        // if the particle is still inside our total bounds, then re-add it
        AACube treeBounds = getRoot()->getAACube();

        if (!shouldDie && treeBounds.contains(args._movingEntities[i].getPosition())) {
            storeEntity(args._movingEntities[i]);
        } else {
            uint32_t entityItemID = args._movingEntities[i].getID();
            quint64 deletedAt = usecTimestampNow();
            _recentlyDeletedEntitysLock.lockForWrite();
            _recentlyDeletedEntityItemIDs.insert(deletedAt, entityItemID);
            _recentlyDeletedEntitysLock.unlock();
        }
    }

    // prune the tree...
    recurseTreeWithOperation(pruneOperation, NULL);
    unlock();
}


bool EntityTree::hasEntitysDeletedSince(quint64 sinceTime) {
    // we can probably leverage the ordered nature of QMultiMap to do this quickly...
    bool hasSomethingNewer = false;

    _recentlyDeletedEntitysLock.lockForRead();
    QMultiMap<quint64, uint32_t>::const_iterator iterator = _recentlyDeletedEntityItemIDs.constBegin();
    while (iterator != _recentlyDeletedEntityItemIDs.constEnd()) {
        //qDebug() << "considering... time/key:" << iterator.key();
        if (iterator.key() > sinceTime) {
            //qDebug() << "YES newer... time/key:" << iterator.key();
            hasSomethingNewer = true;
        }
        ++iterator;
    }
    _recentlyDeletedEntitysLock.unlock();
    return hasSomethingNewer;
}

// sinceTime is an in/out parameter - it will be side effected with the last time sent out
bool EntityTree::encodeEntitysDeletedSince(OCTREE_PACKET_SEQUENCE sequenceNumber, quint64& sinceTime, unsigned char* outputBuffer,
                                            size_t maxLength, size_t& outputLength) {

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
    
    // we keep a multi map of model IDs to timestamps, we only want to include the model IDs that have been
    // deleted since we last sent to this node
    _recentlyDeletedEntitysLock.lockForRead();
    QMultiMap<quint64, uint32_t>::const_iterator iterator = _recentlyDeletedEntityItemIDs.constBegin();
    while (iterator != _recentlyDeletedEntityItemIDs.constEnd()) {
        QList<uint32_t> values = _recentlyDeletedEntityItemIDs.values(iterator.key());
        for (int valueItem = 0; valueItem < values.size(); ++valueItem) {

            // if the timestamp is more recent then out last sent time, include it
            if (iterator.key() > sinceTime) {
                uint32_t modelID = values.at(valueItem);
                memcpy(copyAt, &modelID, sizeof(modelID));
                copyAt += sizeof(modelID);
                outputLength += sizeof(modelID);
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
    _recentlyDeletedEntitysLock.unlock();

    // replace the correct count for ids included
    memcpy(numberOfIDsAt, &numberOfIds, sizeof(numberOfIds));
    return hasMoreToSend;
}

// called by the server when it knows all nodes have been sent deleted packets

void EntityTree::forgetEntitysDeletedBefore(quint64 sinceTime) {
    //qDebug() << "forgetEntitysDeletedBefore()";
    QSet<quint64> keysToRemove;

    _recentlyDeletedEntitysLock.lockForWrite();
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
    
    _recentlyDeletedEntitysLock.unlock();
}


void EntityTree::processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {
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
        QSet<EntityItemID> modelItemIDsToDelete;

        for (size_t i = 0; i < numberOfIds; i++) {
            if (processedBytes + sizeof(uint32_t) > packetLength) {
                break; // bail to prevent buffer overflow
            }

            uint32_t modelID = 0; // placeholder for now
            memcpy(&modelID, dataAt, sizeof(modelID));
            dataAt += sizeof(modelID);
            processedBytes += sizeof(modelID);
            
            EntityItemID entityItemID(modelID);
            modelItemIDsToDelete << entityItemID;
        }
        deleteEntitys(modelItemIDsToDelete);
    }
}


EntityTreeElement* EntityTree::getContainingElement(const EntityItemID& entityItemID) const {
    //qDebug() << "_modelToElementMap=" << _modelToElementMap;

    // TODO: do we need to make this thread safe? Or is it acceptable as is
    if (_modelToElementMap.contains(entityItemID)) {
        return _modelToElementMap.value(entityItemID);
    }
    return NULL;
}

void EntityTree::setContainingElement(const EntityItemID& entityItemID, EntityTreeElement* element) {
    // TODO: do we need to make this thread safe? Or is it acceptable as is
    if (element) {
        _modelToElementMap[entityItemID] = element;
    } else {
        _modelToElementMap.remove(entityItemID);
    }

    //qDebug() << "setContainingElement() entityItemID=" << entityItemID << "element=" << element;
    //qDebug() << "AFTER _modelToElementMap=" << _modelToElementMap;
}

void EntityTree::debugDumpMap() {
    QHashIterator<EntityItemID, EntityTreeElement*> i(_modelToElementMap);
    while (i.hasNext()) {
        i.next();
        qDebug() << i.key() << ": " << i.value();
    }
}
