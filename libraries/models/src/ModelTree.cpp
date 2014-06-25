//
//  ModelTree.cpp
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelTree.h"

ModelTree::ModelTree(bool shouldReaverage) : Octree(shouldReaverage) {
    _rootElement = createNewElement();
}

ModelTreeElement* ModelTree::createNewElement(unsigned char * octalCode) {
    ModelTreeElement* newElement = new ModelTreeElement(octalCode);
    newElement->setTree(this);
    return newElement;
}

void ModelTree::eraseAllOctreeElements() {
    _modelToElementMap.clear();
    Octree::eraseAllOctreeElements();
}

bool ModelTree::handlesEditPacketType(PacketType packetType) const {
    // we handle these types of "edit" packets
    switch (packetType) {
        case PacketTypeModelAddOrEdit:
        case PacketTypeModelErase:
            return true;
        default:
            return false;
    }
}

class StoreModelOperator : public RecurseOctreeOperator {
public:
    StoreModelOperator(ModelTree* tree, const ModelItem& searchModel);
    virtual bool PreRecursion(OctreeElement* element);
    virtual bool PostRecursion(OctreeElement* element);
    virtual OctreeElement* PossiblyCreateChildAt(OctreeElement* element, int childIndex);
private:
    ModelTree* _tree;
    const ModelItem& _newModel;
    const ModelItem* _oldModel;
    ModelTreeElement* _containingElement;
    bool _foundOld;
    bool _foundNew;
    quint64 _changeTime;

    bool subTreeContainsOldModel(OctreeElement* element);
    bool subTreeContainsNewModel(OctreeElement* element);
};

StoreModelOperator::StoreModelOperator(ModelTree* tree, const ModelItem& searchModel) :
    _tree(tree),
    _newModel(searchModel),
    _oldModel(NULL),
    _containingElement(NULL),
    _foundOld(false),
    _foundNew(false),
    _changeTime(usecTimestampNow())
{
    // check our tree, to determine if this model is known
    _containingElement = _tree->getContainingElement(searchModel.getModelItemID());
    if (_containingElement) {
        _oldModel = _containingElement->getModelWithModelItemID(searchModel.getModelItemID());
        if (!_oldModel) {
            //assert(_oldModel);
            qDebug() << "that's UNEXPECTED, we got a _containingElement, but couldn't find the oldModel!";
        }
        
        // If this containing element would be the best fit for our new model, then just do the new
        // portion of the store pass, since the change path will be the same for both parts of the update
        if (_containingElement->bestFitModelBounds(_newModel)) {
            _foundOld = true;
        }
    } else {
        // if the old model is not known, then we can consider if found, and
        // we'll only be searching for the new location
        _foundOld = true;
    }
}

// does this model tree element contain the old model
bool StoreModelOperator::subTreeContainsOldModel(OctreeElement* element) {
    bool containsModel = false;

    // If we don't have an old model, then we don't contain the model, otherwise
    // check the bounds
    if (_oldModel) {
        AACube elementCube = element->getAACube();
        AACube modelCube = _oldModel->getAACube();
        containsModel = elementCube.contains(modelCube);
    }
    return containsModel;
}

bool StoreModelOperator::subTreeContainsNewModel(OctreeElement* element) {
    AACube elementCube = element->getAACube();
    AACube modelCube = _newModel.getAACube();
    return elementCube.contains(modelCube);
}


bool StoreModelOperator::PreRecursion(OctreeElement* element) {
    ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);
    
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
    if (!_foundOld && subTreeContainsOldModel(element)) {
        
        // If this is the element we're looking for, then ask it to remove the old model
        // and we can stop searching.
        if (modelTreeElement == _containingElement) {
        
            // If the containgElement IS NOT the best fit for the new model properties
            // then we need to remove it, and the updateModel below will store it in the
            // correct element.
            if (!_containingElement->bestFitModelBounds(_newModel)) {
                modelTreeElement->removeModelWithModelItemID(_newModel.getModelItemID());
                
                // If we haven't yet found the new location, then we need to 
                // make sure to remove our model to element map, because for
                // now we're not in that map
                if (!_foundNew) {
                    _tree->setContainingElement(_newModel.getModelItemID(), NULL);
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
    if (!_foundNew && subTreeContainsNewModel(element)) {
    
        // Note: updateModel() will only operate on correctly found models and/or add them
        // to the element if they SHOULD be stored there.
        if (modelTreeElement->updateModel(_newModel)) {
            //qDebug() << "StoreModelOperator::PreRecursion()... model was updated!";
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

bool StoreModelOperator::PostRecursion(OctreeElement* element) {
    // Post-recursion is the unwinding process. For this operation, while we
    // unwind we want to mark the path as being dirty if we changed it below.
    // We might have two paths, one for the old model and one for the new model.
    bool keepSearching = !_foundOld || !_foundNew;

    // As we unwind, if we're in either of these two paths, we mark our element
    // as dirty.
    if ((_foundOld && subTreeContainsOldModel(element)) ||
            (_foundNew && subTreeContainsNewModel(element))) {
        element->markWithChangedTime();
    }
    return keepSearching; // if we haven't yet found it, keep looking
}

OctreeElement* StoreModelOperator::PossiblyCreateChildAt(OctreeElement* element, int childIndex) { 
    // If we're getting called, it's because there was no child element at this index while recursing.
    // We only care if this happens while still searching for the new model location.
    // Check to see if 
    if (!_foundNew) {
        int indexOfChildContainingNewModel = element->getMyChildContaining(_newModel.getAACube());
        
        if (childIndex == indexOfChildContainingNewModel) {
            return element->addChildAtIndex(childIndex);
        }
    }
    return NULL; 
}


void ModelTree::storeModel(const ModelItem& model, const SharedNodePointer& senderNode) {
    // NOTE: callers must lock the tree before using this method

    // First, look for the existing model in the tree..
    StoreModelOperator theOperator(this, model);

    recurseTreeWithOperator(&theOperator);
    _isDirty = true;

    bool wantDebug = false;
    if (wantDebug) {
        ModelTreeElement* containingElement = getContainingElement(model.getModelItemID());
        qDebug() << "ModelTree::storeModel().... after store... containingElement=" << containingElement;
    }

}

void ModelTree::updateModel(const ModelItemID& modelID, const ModelItemProperties& properties) {
    ModelItem updateItem(modelID);

    bool wantDebug = false;
    if (wantDebug) {    
        qDebug() << "ModelTree::updateModel(modelID, properties) line:" << __LINE__ << "updateItem:";
        updateItem.debugDump();
    }
    
    // since the properties might not be complete, they may only contain some values,
    // we need to first see if we already have the model in our tree, and make a copy of
    // its existing properties first
    ModelTreeElement* containingElement = getContainingElement(modelID);

    if (wantDebug) {    
        qDebug() << "ModelTree::updateModel(modelID, properties) containingElement=" << containingElement;
    }

    if (containingElement) {
        const ModelItem* oldModel = containingElement->getModelWithModelItemID(modelID);
        if (oldModel) {
            ModelItemProperties oldProps = oldModel->getProperties();
            if (wantDebug) {    
                qDebug() << "ModelTree::updateModel(modelID, properties) ********** COPY PROPERTIES FROM oldModel=" << oldModel << "*******************";
                qDebug() << "ModelTree::updateModel(modelID, properties) oldModel=" << oldModel;
                oldProps.debugDump();
                qDebug() << "ModelTree::updateModel(modelID, properties) line:" << __LINE__ << "about to call updateItem.setProperties(oldProps);";
            }
            updateItem.setProperties(oldProps, true); // force copy

            if (wantDebug) {    
                qDebug() << "ModelTree::updateModel(modelID, properties) line:" << __LINE__ << "updateItem:";
                updateItem.debugDump();
            }

        } else {
            if (wantDebug) {    
                qDebug() << "ModelTree::updateModel(modelID, properties) WAIT WHAT!!! COULDN'T FIND oldModel=" << oldModel;
            }
        }
    }
    updateItem.setProperties(properties);

    if (wantDebug) {    
        qDebug() << "ModelTree::updateModel(modelID, properties) line:" << __LINE__ << "updateItem:";
        updateItem.debugDump();
    }

    storeModel(updateItem);
}

void ModelTree::addModel(const ModelItemID& modelID, const ModelItemProperties& properties) {
    ModelItem updateItem(modelID, properties);
    storeModel(updateItem);
}

class DeleteModelOperator : public RecurseOctreeOperator {
public:
    DeleteModelOperator(ModelTree* tree, const ModelItemID& searchModelID);
    virtual bool PreRecursion(OctreeElement* element);
    virtual bool PostRecursion(OctreeElement* element);
private:
    ModelTree* _tree;
    const ModelItem* _oldModel;
    AACube _oldModelCube;
    ModelTreeElement* _containingElement;
    bool _foundOld;
    quint64 _changeTime;
    bool subTreeContainsOldModel(OctreeElement* element);
    bool subTreeContainsNewModel(OctreeElement* element);
};

DeleteModelOperator::DeleteModelOperator(ModelTree* tree, const ModelItemID& searchModelID) :
    _tree(tree),
    _oldModel(NULL),
    _containingElement(NULL),
    _foundOld(false),
    _changeTime(usecTimestampNow())
{
    // check our tree, to determine if this model is known
    _containingElement = _tree->getContainingElement(searchModelID);
    if (_containingElement) {
        _oldModel = _containingElement->getModelWithModelItemID(searchModelID);
        if (!_oldModel) {
            //assert(_oldModel);
            qDebug() << "that's UNEXPECTED, we got a _containingElement, but couldn't find the oldModel!";
        }
        _oldModelCube = _oldModel->getAACube();
    } else {
        // if the old model is not known, then we can consider if found, and
        // we'll only be searching for the new location
        _foundOld = true;
    }
}

// does this model tree element contain the old model
bool DeleteModelOperator::subTreeContainsOldModel(OctreeElement* element) {
    bool containsModel = false;

    // If we don't have an old model, then we don't contain the model, otherwise
    // check the bounds
    if (_oldModel) {
        AACube elementCube = element->getAACube();
        containsModel = elementCube.contains(_oldModelCube);
    }
    return containsModel;
}

bool DeleteModelOperator::PreRecursion(OctreeElement* element) {
    ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);
    
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
    if (!_foundOld && subTreeContainsOldModel(element)) {
        
        // If this is the element we're looking for, then ask it to remove the old model
        // and we can stop searching.
        if (modelTreeElement == _containingElement) {

            // This is a good place to delete it!!!
            ModelItemID modelItemID = _oldModel->getModelItemID();
            modelTreeElement->removeModelWithModelItemID(modelItemID);
            _tree->setContainingElement(modelItemID, NULL);
            _foundOld = true;
        } else {
            // if this isn't the element we're looking for, then keep searching
            keepSearching = true;
        }
    }

    return keepSearching; // if we haven't yet found it, keep looking
}

bool DeleteModelOperator::PostRecursion(OctreeElement* element) {
    // Post-recursion is the unwinding process. For this operation, while we
    // unwind we want to mark the path as being dirty if we changed it below.
    // We might have two paths, one for the old model and one for the new model.
    bool keepSearching = !_foundOld;

    // As we unwind, if we're in either of these two paths, we mark our element
    // as dirty.
    if ((_foundOld && subTreeContainsOldModel(element))) {
        element->markWithChangedTime();
    }
    return keepSearching; // if we haven't yet found it, keep looking
}

void ModelTree::deleteModel(const ModelItemID& modelID) {
    // NOTE: callers must lock the tree before using this method

    // First, look for the existing model in the tree..
    DeleteModelOperator theOperator(this, modelID);

    recurseTreeWithOperator(&theOperator);
    _isDirty = true;

    bool wantDebug = false;
    if (wantDebug) {
        ModelTreeElement* containingElement = getContainingElement(modelID);
        qDebug() << "ModelTree::storeModel().... after store... containingElement=" << containingElement;
    }
}

void ModelTree::deleteModels(QSet<ModelItemID> modelIDs) {
    // NOTE: callers must lock the tree before using this method

    // TODO: fix this to use one pass
    foreach(const ModelItemID& modelID, modelIDs) {

        // First, look for the existing model in the tree..
        DeleteModelOperator theOperator(this, modelID);

        recurseTreeWithOperator(&theOperator);
        _isDirty = true;

        bool wantDebug = false;
        if (wantDebug) {
            ModelTreeElement* containingElement = getContainingElement(modelID);
            qDebug() << "ModelTree::storeModel().... after store... containingElement=" << containingElement;
        }
    }
}

// scans the tree and handles mapping locally created models to know IDs.
// in the event that this tree is also viewing the scene, then we need to also
// search the tree to make sure we don't have a duplicate model from the viewing
// operation.
bool ModelTree::findAndUpdateModelItemIDOperation(OctreeElement* element, void* extraData) {
    bool keepSearching = true;

    FindAndUpdateModelItemIDArgs* args = static_cast<FindAndUpdateModelItemIDArgs*>(extraData);
    ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);

    // Note: updateModelItemID() will only operate on correctly found models
    modelTreeElement->updateModelItemID(args);

    // if we've found and replaced both the creatorTokenID and the viewedModel, then we
    // can stop looking, otherwise we will keep looking    
    if (args->creatorTokenFound && args->viewedModelFound) {
        keepSearching = false;
    }
    
    return keepSearching;
}

void ModelTree::handleAddModelResponse(const QByteArray& packet) {
    const bool wantDebug = false;

    if (wantDebug) {
        qDebug() << "ModelTree::handleAddModelResponse()..."; 
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
    bool assumeModelFound = !getIsViewing(); // if we're not a viewing tree, then we don't have to find the actual model
    FindAndUpdateModelItemIDArgs args = { 
        modelID, 
        creatorTokenID, 
        false, 
        assumeModelFound,
        getIsViewing() 
    };
    
    if (wantDebug) {
        qDebug() << "looking for creatorTokenID=" << creatorTokenID << " modelID=" << modelID 
                << " getIsViewing()=" << getIsViewing();
    }
    lockForWrite();
    // TODO: Switch this to use list of known model IDs....
    recurseTreeWithOperation(findAndUpdateModelItemIDOperation, &args);
    unlock();
}


class FindNearPointArgs {
public:
    glm::vec3 position;
    float targetRadius;
    bool found;
    const ModelItem* closestModel;
    float closestModelDistance;
};


bool ModelTree::findNearPointOperation(OctreeElement* element, void* extraData) {
    FindNearPointArgs* args = static_cast<FindNearPointArgs*>(extraData);
    ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);

    glm::vec3 penetration;
    bool sphereIntersection = modelTreeElement->getAACube().findSpherePenetration(args->position,
                                                                    args->targetRadius, penetration);

    // If this modelTreeElement contains the point, then search it...
    if (sphereIntersection) {
        const ModelItem* thisClosestModel = modelTreeElement->getClosestModel(args->position);

        // we may have gotten NULL back, meaning no model was available
        if (thisClosestModel) {
            glm::vec3 modelPosition = thisClosestModel->getPosition();
            float distanceFromPointToModel = glm::distance(modelPosition, args->position);

            // If we're within our target radius
            if (distanceFromPointToModel <= args->targetRadius) {
                // we are closer than anything else we've found
                if (distanceFromPointToModel < args->closestModelDistance) {
                    args->closestModel = thisClosestModel;
                    args->closestModelDistance = distanceFromPointToModel;
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

const ModelItem* ModelTree::findClosestModel(glm::vec3 position, float targetRadius) {
    FindNearPointArgs args = { position, targetRadius, false, NULL, FLT_MAX };
    lockForRead();
    // NOTE: This should use recursion, since this is a spatial operation
    recurseTreeWithOperation(findNearPointOperation, &args);
    unlock();
    return args.closestModel;
}

class FindAllNearPointArgs {
public:
    glm::vec3 position;
    float targetRadius;
    QVector<const ModelItem*> models;
};


bool ModelTree::findInSphereOperation(OctreeElement* element, void* extraData) {
    FindAllNearPointArgs* args = static_cast<FindAllNearPointArgs*>(extraData);
    glm::vec3 penetration;
    bool sphereIntersection = element->getAACube().findSpherePenetration(args->position,
                                                                    args->targetRadius, penetration);

    // If this element contains the point, then search it...
    if (sphereIntersection) {
        ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);
        modelTreeElement->getModels(args->position, args->targetRadius, args->models);
        return true; // keep searching in case children have closer models
    }

    // if this element doesn't contain the point, then none of it's children can contain the point, so stop searching
    return false;
}

void ModelTree::findModels(const glm::vec3& center, float radius, QVector<const ModelItem*>& foundModels) {
    FindAllNearPointArgs args = { center, radius };
    lockForRead();
    // NOTE: This should use recursion, since this is a spatial operation
    recurseTreeWithOperation(findInSphereOperation, &args);
    unlock();
    // swap the two lists of model pointers instead of copy
    foundModels.swap(args.models);
}

class FindModelsInCubeArgs {
public:
    FindModelsInCubeArgs(const AACube& cube) 
        : _cube(cube), _foundModels() {
    }

    AACube _cube;
    QVector<ModelItem*> _foundModels;
};

bool ModelTree::findInCubeOperation(OctreeElement* element, void* extraData) {
    FindModelsInCubeArgs* args = static_cast< FindModelsInCubeArgs*>(extraData);
    const AACube& elementCube = element->getAACube();
    if (elementCube.touches(args->_cube)) {
        ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);
        modelTreeElement->getModels(args->_cube, args->_foundModels);
        return true;
    }
    return false;
}

void ModelTree::findModels(const AACube& cube, QVector<ModelItem*> foundModels) {
    FindModelsInCubeArgs args(cube);
    lockForRead();
    // NOTE: This should use recursion, since this is a spatial operation
    recurseTreeWithOperation(findInCubeOperation, &args);
    unlock();
    // swap the two lists of model pointers instead of copy
    foundModels.swap(args._foundModels);
}

const ModelItem* ModelTree::findModelByID(uint32_t id, bool alreadyLocked) const {
    ModelItemID modelID(id);

    bool wantDebug = false;
    if (wantDebug) {
        qDebug() << "ModelTree::findModelByID()...";
        qDebug() << "    id=" << id;
        qDebug() << "    modelID=" << modelID;
        qDebug() << "_modelToElementMap=" << _modelToElementMap;
    }

    return findModelByModelItemID(modelID);
}

const ModelItem* ModelTree::findModelByModelItemID(const ModelItemID& modelID) const {
    const ModelItem* foundModel = NULL;
    ModelTreeElement* containingElement = getContainingElement(modelID);
    if (containingElement) {
        foundModel = containingElement->getModelWithModelItemID(modelID);
    }
    return foundModel;
}


int ModelTree::processEditPacketData(PacketType packetType, const unsigned char* packetData, int packetLength,
                    const unsigned char* editData, int maxLength, const SharedNodePointer& senderNode) {

    int processedBytes = 0;
    // we handle these types of "edit" packets
    switch (packetType) {
        case PacketTypeModelAddOrEdit: {
            bool isValid;
            ModelItem newModel = ModelItem::fromEditPacket(editData, maxLength, processedBytes, this, isValid);
            if (isValid) {
                storeModel(newModel, senderNode);
                if (newModel.isNewlyCreated()) {
                    notifyNewlyCreatedModel(newModel, senderNode);
                }
            }
        } break;

        default:
            processedBytes = 0;
            break;
    }
    
    return processedBytes;
}

void ModelTree::notifyNewlyCreatedModel(const ModelItem& newModel, const SharedNodePointer& senderNode) {
    _newlyCreatedHooksLock.lockForRead();
    for (size_t i = 0; i < _newlyCreatedHooks.size(); i++) {
        _newlyCreatedHooks[i]->modelCreated(newModel, senderNode);
    }
    _newlyCreatedHooksLock.unlock();
}

void ModelTree::addNewlyCreatedHook(NewlyCreatedModelHook* hook) {
    _newlyCreatedHooksLock.lockForWrite();
    _newlyCreatedHooks.push_back(hook);
    _newlyCreatedHooksLock.unlock();
}

void ModelTree::removeNewlyCreatedHook(NewlyCreatedModelHook* hook) {
    _newlyCreatedHooksLock.lockForWrite();
    for (size_t i = 0; i < _newlyCreatedHooks.size(); i++) {
        if (_newlyCreatedHooks[i] == hook) {
            _newlyCreatedHooks.erase(_newlyCreatedHooks.begin() + i);
            break;
        }
    }
    _newlyCreatedHooksLock.unlock();
}


bool ModelTree::updateOperation(OctreeElement* element, void* extraData) {
    ModelTreeUpdateArgs* args = static_cast<ModelTreeUpdateArgs*>(extraData);
    ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);
    modelTreeElement->update(*args);
    return true;
}

bool ModelTree::pruneOperation(OctreeElement* element, void* extraData) {
    ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        ModelTreeElement* childAt = modelTreeElement->getChildAtIndex(i);
        if (childAt && childAt->isLeaf() && !childAt->hasModels()) {
            modelTreeElement->deleteChildAtIndex(i);
        }
    }
    return true;
}

void ModelTree::update() {
    lockForWrite();
    _isDirty = true;

    // TODO: could we manage this by iterating the known models map/hash? Would that be faster?
    ModelTreeUpdateArgs args;
    recurseTreeWithOperation(updateOperation, &args);

    // now add back any of the particles that moved elements....
    int movingModels = args._movingModels.size();
    
    for (int i = 0; i < movingModels; i++) {
        bool shouldDie = args._movingModels[i].getShouldDie();

        // if the particle is still inside our total bounds, then re-add it
        AACube treeBounds = getRoot()->getAACube();

        if (!shouldDie && treeBounds.contains(args._movingModels[i].getPosition())) {
            storeModel(args._movingModels[i]);
        } else {
            uint32_t modelItemID = args._movingModels[i].getID();
            quint64 deletedAt = usecTimestampNow();
            _recentlyDeletedModelsLock.lockForWrite();
            _recentlyDeletedModelItemIDs.insert(deletedAt, modelItemID);
            _recentlyDeletedModelsLock.unlock();
        }
    }

    // prune the tree...
    recurseTreeWithOperation(pruneOperation, NULL);
    unlock();
}


bool ModelTree::hasModelsDeletedSince(quint64 sinceTime) {
    // we can probably leverage the ordered nature of QMultiMap to do this quickly...
    bool hasSomethingNewer = false;

    _recentlyDeletedModelsLock.lockForRead();
    QMultiMap<quint64, uint32_t>::const_iterator iterator = _recentlyDeletedModelItemIDs.constBegin();
    while (iterator != _recentlyDeletedModelItemIDs.constEnd()) {
        //qDebug() << "considering... time/key:" << iterator.key();
        if (iterator.key() > sinceTime) {
            //qDebug() << "YES newer... time/key:" << iterator.key();
            hasSomethingNewer = true;
        }
        ++iterator;
    }
    _recentlyDeletedModelsLock.unlock();
    return hasSomethingNewer;
}

// sinceTime is an in/out parameter - it will be side effected with the last time sent out
bool ModelTree::encodeModelsDeletedSince(OCTREE_PACKET_SEQUENCE sequenceNumber, quint64& sinceTime, unsigned char* outputBuffer,
                                            size_t maxLength, size_t& outputLength) {

    bool hasMoreToSend = true;

    unsigned char* copyAt = outputBuffer;
    size_t numBytesPacketHeader = populatePacketHeader(reinterpret_cast<char*>(outputBuffer), PacketTypeModelErase);
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
    _recentlyDeletedModelsLock.lockForRead();
    QMultiMap<quint64, uint32_t>::const_iterator iterator = _recentlyDeletedModelItemIDs.constBegin();
    while (iterator != _recentlyDeletedModelItemIDs.constEnd()) {
        QList<uint32_t> values = _recentlyDeletedModelItemIDs.values(iterator.key());
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
    if (iterator == _recentlyDeletedModelItemIDs.constEnd()) {
        hasMoreToSend = false;
    }
    _recentlyDeletedModelsLock.unlock();

    // replace the correct count for ids included
    memcpy(numberOfIDsAt, &numberOfIds, sizeof(numberOfIds));
    return hasMoreToSend;
}

// called by the server when it knows all nodes have been sent deleted packets

void ModelTree::forgetModelsDeletedBefore(quint64 sinceTime) {
    //qDebug() << "forgetModelsDeletedBefore()";
    QSet<quint64> keysToRemove;

    _recentlyDeletedModelsLock.lockForWrite();
    QMultiMap<quint64, uint32_t>::iterator iterator = _recentlyDeletedModelItemIDs.begin();

    // First find all the keys in the map that are older and need to be deleted
    while (iterator != _recentlyDeletedModelItemIDs.end()) {
        if (iterator.key() <= sinceTime) {
            keysToRemove << iterator.key();
        }
        ++iterator;
    }

    // Now run through the keysToRemove and remove them
    foreach (quint64 value, keysToRemove) {
        //qDebug() << "removing the key, _recentlyDeletedModelItemIDs.remove(value); time/key:" << value;
        _recentlyDeletedModelItemIDs.remove(value);
    }
    
    _recentlyDeletedModelsLock.unlock();
}


void ModelTree::processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {
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
        QSet<ModelItemID> modelItemIDsToDelete;

        for (size_t i = 0; i < numberOfIds; i++) {
            if (processedBytes + sizeof(uint32_t) > packetLength) {
                break; // bail to prevent buffer overflow
            }

            uint32_t modelID = 0; // placeholder for now
            memcpy(&modelID, dataAt, sizeof(modelID));
            dataAt += sizeof(modelID);
            processedBytes += sizeof(modelID);
            
            ModelItemID modelItemID(modelID);
            modelItemIDsToDelete << modelItemID;
        }
        deleteModels(modelItemIDsToDelete);
    }
}


ModelTreeElement* ModelTree::getContainingElement(const ModelItemID& modelItemID) const {
    //qDebug() << "_modelToElementMap=" << _modelToElementMap;

    // TODO: do we need to make this thread safe? Or is it acceptable as is
    if (_modelToElementMap.contains(modelItemID)) {
        return _modelToElementMap.value(modelItemID);
    }
    return NULL;
}

void ModelTree::setContainingElement(const ModelItemID& modelItemID, ModelTreeElement* element) {
    // TODO: do we need to make this thread safe? Or is it acceptable as is
    if (element) {
        _modelToElementMap[modelItemID] = element;
    } else {
        _modelToElementMap.remove(modelItemID);
    }

    //qDebug() << "setContainingElement() modelItemID=" << modelItemID << "element=" << element;
    //qDebug() << "AFTER _modelToElementMap=" << _modelToElementMap;
}

void ModelTree::debugDumpMap() {
    QHashIterator<ModelItemID, ModelTreeElement*> i(_modelToElementMap);
    while (i.hasNext()) {
        i.next();
        qDebug() << i.key() << ": " << i.value();
    }
}
