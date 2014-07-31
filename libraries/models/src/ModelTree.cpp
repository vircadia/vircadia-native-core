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

#include "ModelEditPacketSender.h"
#include "ModelItem.h"

#include "ModelTree.h"

ModelTree::ModelTree(bool shouldReaverage) : Octree(shouldReaverage) {
    _rootElement = createNewElement();
}

ModelTreeElement* ModelTree::createNewElement(unsigned char * octalCode) {
    ModelTreeElement* newElement = new ModelTreeElement(octalCode);
    newElement->setTree(this);
    return newElement;
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

class FindAndDeleteModelsArgs {
public:
    QList<uint32_t> _idsToDelete;
};

bool ModelTree::findAndDeleteOperation(OctreeElement* element, void* extraData) {
    //qDebug() << "findAndDeleteOperation()";

    FindAndDeleteModelsArgs* args = static_cast< FindAndDeleteModelsArgs*>(extraData);

    // if we've found and deleted all our target models, then we can stop looking
    if (args->_idsToDelete.size() <= 0) {
        return false;
    }

    ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);

    //qDebug() << "findAndDeleteOperation() args->_idsToDelete.size():" << args->_idsToDelete.size();

    for (QList<uint32_t>::iterator it = args->_idsToDelete.begin(); it != args->_idsToDelete.end(); it++) {
        uint32_t modelID = *it;
        //qDebug() << "findAndDeleteOperation() modelID:" << modelID;

        if (modelTreeElement->removeModelWithID(modelID)) {
            // if the model was in this element, then remove it from our search list.
            //qDebug() << "findAndDeleteOperation() it = args->_idsToDelete.erase(it)";
            it = args->_idsToDelete.erase(it);
        }

        if (it == args->_idsToDelete.end()) {
            //qDebug() << "findAndDeleteOperation() breaking";
            break;
        }
    }

    // if we've found and deleted all our target models, then we can stop looking
    if (args->_idsToDelete.size() <= 0) {
        return false;
    }
    return true;
}

class FindAndUpdateModelOperator : public RecurseOctreeOperator {
public:
    FindAndUpdateModelOperator(const ModelItem& searchModel);
    virtual bool PreRecursion(OctreeElement* element);
    virtual bool PostRecursion(OctreeElement* element);
    bool wasFound() const { return _found; }
private:
    const ModelItem& _searchModel;
    bool _found;
};

FindAndUpdateModelOperator::FindAndUpdateModelOperator(const ModelItem& searchModel) :
    _searchModel(searchModel),
    _found(false) {
};

bool FindAndUpdateModelOperator::PreRecursion(OctreeElement* element) {
    ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);
    // Note: updateModel() will only operate on correctly found models
    if (modelTreeElement->updateModel(_searchModel)) {
        _found = true;
        return false; // stop searching
    }

    return !_found; // if we haven't yet found it, keep looking
}

bool FindAndUpdateModelOperator::PostRecursion(OctreeElement* element) {
    if (_found) {
        element->markWithChangedTime();
    }
    return !_found; // if we haven't yet found it, keep looking
}


ModelTreeElement* ModelTree::getModelAt(float x, float y, float z, float s) const {
    return static_cast<ModelTreeElement*>(getOctreeElementAt(x, y, z, s));
}

// TODO: improve this to not use multiple recursions
void ModelTree::storeModel(const ModelItem& model, const SharedNodePointer& senderNode) {
    // First, look for the existing model in the tree..
    FindAndUpdateModelOperator theOperator(model);
    recurseTreeWithOperator(&theOperator);
    
    // if we didn't find it in the tree, then store it...
    if (!theOperator.wasFound()) {
        AACube modelCube = model.getAACube();
        ModelTreeElement* element = static_cast<ModelTreeElement*>(getOrCreateChildElementContaining(model.getAACube()));
        element->storeModel(model);
        
        // In the case where we stored it, we also need to mark the entire "path" down to the model as
        // having changed. Otherwise viewers won't see this change. So we call this recursion now that
        // we know it will be found, this find/update will correctly mark the tree as changed.
        recurseTreeWithOperator(&theOperator);
    }

    _isDirty = true;
}


class FindAndUpdateModelWithIDandPropertiesOperator : public RecurseOctreeOperator {
public:
    FindAndUpdateModelWithIDandPropertiesOperator(const ModelItemID& modelID, const ModelItemProperties& properties);
    virtual bool PreRecursion(OctreeElement* element);
    virtual bool PostRecursion(OctreeElement* element);
    bool wasFound() const { return _found; }
private:
    const ModelItemID& _modelID;
    const ModelItemProperties& _properties;
    bool _found;
};

FindAndUpdateModelWithIDandPropertiesOperator::FindAndUpdateModelWithIDandPropertiesOperator(const ModelItemID& modelID, 
    const ModelItemProperties& properties) :
    _modelID(modelID),
    _properties(properties),
    _found(false) {
};

bool FindAndUpdateModelWithIDandPropertiesOperator::PreRecursion(OctreeElement* element) {
    ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);

    // Note: updateModel() will only operate on correctly found models
    if (modelTreeElement->updateModel(_modelID, _properties)) {
        _found = true;
        return false; // stop searching
    }
    return !_found; // if we haven't yet found it, keep looking
}

bool FindAndUpdateModelWithIDandPropertiesOperator::PostRecursion(OctreeElement* element) {
    if (_found) {
        element->markWithChangedTime();
    }
    return !_found; // if we haven't yet found it, keep looking
}

void ModelTree::updateModel(const ModelItemID& modelID, const ModelItemProperties& properties) {
    // Look for the existing model in the tree..
    FindAndUpdateModelWithIDandPropertiesOperator theOperator(modelID, properties);
    recurseTreeWithOperator(&theOperator);
    if (theOperator.wasFound()) {
        _isDirty = true;
    }
}

void ModelTree::addModel(const ModelItemID& modelID, const ModelItemProperties& properties) {
    // This only operates on locally created models
    if (modelID.isKnownID) {
        return; // not allowed
    }
    ModelItem model(modelID, properties);
    glm::vec3 position = model.getPosition();
    float size = std::max(MINIMUM_MODEL_ELEMENT_SIZE, model.getRadius());
    
    ModelTreeElement* element = static_cast<ModelTreeElement*>(getOrCreateChildElementAt(position.x, position.y, position.z, size));
    element->storeModel(model);
    
    _isDirty = true;
}

void ModelTree::deleteModel(const ModelItemID& modelID) {
    if (modelID.isKnownID) {
        FindAndDeleteModelsArgs args;
        args._idsToDelete.push_back(modelID.id);
        recurseTreeWithOperation(findAndDeleteOperation, &args);
    }
}

void ModelTree::sendModels(ModelEditPacketSender* packetSender, float x, float y, float z) {
    SendModelsOperationArgs args;
    args.packetSender = packetSender;
    args.root = glm::vec3(x, y, z);
    recurseTreeWithOperation(sendModelsOperation, &args);
    packetSender->releaseQueuedMessages();
}

bool ModelTree::sendModelsOperation(OctreeElement* element, void* extraData) {
    SendModelsOperationArgs* args = static_cast<SendModelsOperationArgs*>(extraData);
    ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);

    const QList<ModelItem>& modelList = modelTreeElement->getModels();

    for (int i = 0; i < modelList.size(); i++) {
        uint32_t creatorTokenID = ModelItem::getNextCreatorTokenID();
        ModelItemID id(NEW_MODEL, creatorTokenID, false);
        ModelItemProperties properties;
        properties.copyFromModelItem(modelList.at(i));
        properties.setPosition(properties.getPosition() + args->root);
        args->packetSender->queueModelEditMessage(PacketTypeModelAddOrEdit, id, properties);
    }

    return true;
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
    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    
    const unsigned char* dataAt = reinterpret_cast<const unsigned char*>(packet.data()) + numBytesPacketHeader;

    uint32_t creatorTokenID;
    memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
    dataAt += sizeof(creatorTokenID);

    uint32_t modelID;
    memcpy(&modelID, dataAt, sizeof(modelID));
    dataAt += sizeof(modelID);

    // update models in our tree
    bool assumeModelFound = !getIsViewing(); // if we're not a viewing tree, then we don't have to find the actual model
    FindAndUpdateModelItemIDArgs args = { 
        modelID, 
        creatorTokenID, 
        false, 
        assumeModelFound,
        getIsViewing() 
    };
    
    const bool wantDebug = false;
    if (wantDebug) {
        qDebug() << "looking for creatorTokenID=" << creatorTokenID << " modelID=" << modelID 
                << " getIsViewing()=" << getIsViewing();
    }
    lockForWrite();
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

void ModelTree::findModelsInCube(const AACube& cube, QVector<ModelItem*>& foundModels) {
    FindModelsInCubeArgs args(cube);
    lockForRead();
    recurseTreeWithOperation(findInCubeOperation, &args);
    unlock();
    // swap the two lists of model pointers instead of copy
    foundModels.swap(args._foundModels);
}

bool ModelTree::findInCubeOperation(OctreeElement* element, void* extraData) {
    FindModelsInCubeArgs* args = static_cast< FindModelsInCubeArgs*>(extraData);
    const AACube& elementCube = element->getAACube();
    if (elementCube.touches(args->_cube)) {
        ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);
        modelTreeElement->getModelsInside(args->_cube, args->_foundModels);
        return true;
    }
    return false;
}

bool ModelTree::findInCubeForUpdateOperation(OctreeElement* element, void* extraData) {
    FindModelsInCubeArgs* args = static_cast< FindModelsInCubeArgs*>(extraData);
    const AACube& elementCube = element->getAACube();
    if (elementCube.touches(args->_cube)) {
        ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);
        modelTreeElement->getModelsForUpdate(args->_cube, args->_foundModels);
        return true;
    }
    return false;
}

void ModelTree::findModelsForUpdate(const AACube& cube, QVector<ModelItem*>& foundModels) {
    FindModelsInCubeArgs args(cube);
    lockForRead();
    recurseTreeWithOperation(findInCubeForUpdateOperation, &args);
    unlock();
    // swap the two lists of model pointers instead of copy
    foundModels.swap(args._foundModels);
}

class FindByIDArgs {
public:
    uint32_t id;
    bool found;
    const ModelItem* foundModel;
};


bool ModelTree::findByIDOperation(OctreeElement* element, void* extraData) {
//qDebug() << "ModelTree::findByIDOperation()....";

    FindByIDArgs* args = static_cast<FindByIDArgs*>(extraData);
    ModelTreeElement* modelTreeElement = static_cast<ModelTreeElement*>(element);

    // if already found, stop looking
    if (args->found) {
        return false;
    }

    // as the tree element if it has this model
    const ModelItem* foundModel = modelTreeElement->getModelWithID(args->id);
    if (foundModel) {
        args->foundModel = foundModel;
        args->found = true;
        return false;
    }

    // keep looking
    return true;
}


const ModelItem* ModelTree::findModelByID(uint32_t id, bool alreadyLocked) {
    FindByIDArgs args = { id, false, NULL };

    if (!alreadyLocked) {
        lockForRead();
    }
    recurseTreeWithOperation(findByIDOperation, &args);
    if (!alreadyLocked) {
        unlock();
    }
    return args.foundModel;
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

        // TODO: wire in support here for server to get PacketTypeModelErase messages
        // instead of using PacketTypeModelAddOrEdit messages to delete models
        case PacketTypeModelErase:
            processedBytes = 0;
            break;
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
        FindAndDeleteModelsArgs args;

        for (size_t i = 0; i < numberOfIds; i++) {
            if (processedBytes + sizeof(uint32_t) > packetLength) {
                break; // bail to prevent buffer overflow
            }

            uint32_t modelID = 0; // placeholder for now
            memcpy(&modelID, dataAt, sizeof(modelID));
            dataAt += sizeof(modelID);
            processedBytes += sizeof(modelID);

            args._idsToDelete.push_back(modelID);
        }

        // calling recurse to actually delete the models
        recurseTreeWithOperation(findAndDeleteOperation, &args);
    }
}
