//
//  ModelReferential.cpp
//
//
//  Created by Clement on 7/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <AvatarData.h>
#include <EntityTree.h>
#include <Model.h>

#include "InterfaceLogging.h"
#include "ModelReferential.h"

ModelReferential::ModelReferential(Referential* referential, EntityTreePointer tree, AvatarData* avatar) :
    Referential(MODEL, avatar),
    _tree(tree) 
{
    _translation = referential->getTranslation();
    _rotation = referential->getRotation();
    unpackExtraData(reinterpret_cast<unsigned char*>(referential->getExtraData().data()),
                    referential->getExtraData().size());
    
    if (!isValid()) {
        qCDebug(interfaceapp) << "ModelReferential::copyConstructor(): Not Valid";
        return;
    }
    
    EntityItemPointer item = _tree->findEntityByID(_entityID);
    if (item != NULL) {
        _lastRefDimension = item->getDimensions();
        _refRotation = item->getRotation();
        _refPosition = item->getPosition();
        update();
    }
}

ModelReferential::ModelReferential(const QUuid& entityID, EntityTreePointer tree, AvatarData* avatar) :
    Referential(MODEL, avatar),
    _entityID(entityID),
    _tree(tree)
{
    EntityItemPointer item = _tree->findEntityByID(_entityID);
    if (!isValid() || item == NULL) {
        qCDebug(interfaceapp) << "ModelReferential::constructor(): Not Valid";
        _isValid = false;
        return;
    }
    
    _lastRefDimension = item->getDimensions();
    _refRotation = item->getRotation();
    _refPosition = item->getPosition();
    
    glm::quat refInvRot = glm::inverse(_refRotation);
    _rotation = refInvRot * _avatar->getOrientation();
    _translation = refInvRot * (avatar->getPosition() - _refPosition);
}

void ModelReferential::update() {
    EntityItemPointer item = _tree->findEntityByID(_entityID);
    if (!isValid() || item == NULL || _avatar == NULL) {
        return;
    }
    
    bool somethingChanged = false;
    if (item->getDimensions() != _lastRefDimension) {
        glm::vec3 oldDimension = _lastRefDimension;
        _lastRefDimension = item->getDimensions();
        _translation *= _lastRefDimension / oldDimension;
        somethingChanged = true;
    }
    if (item->getRotation() != _refRotation) {
        _refRotation = item->getRotation();
        _avatar->setOrientation(_refRotation * _rotation, true);
        somethingChanged = true;
    }
    if (item->getPosition() != _refPosition || somethingChanged) {
        _refPosition = item->getPosition();
        _avatar->setPosition(_refPosition + _refRotation * _translation, true);
    }
}

int ModelReferential::packExtraData(unsigned char* destinationBuffer) const {
    QByteArray encodedEntityID = _entityID.toRfc4122();
    memcpy(destinationBuffer, encodedEntityID.constData(), encodedEntityID.size());
    return encodedEntityID.size();
}

int ModelReferential::unpackExtraData(const unsigned char *sourceBuffer, int size) {
    QByteArray encodedEntityID((const char*)sourceBuffer, NUM_BYTES_RFC4122_UUID);
    _entityID = QUuid::fromRfc4122(encodedEntityID);
    return NUM_BYTES_RFC4122_UUID;
}

JointReferential::JointReferential(Referential* referential, EntityTreePointer tree, AvatarData* avatar) :
    ModelReferential(referential, tree, avatar)
{
    _type = JOINT;
    if (!isValid()) {
        qCDebug(interfaceapp) << "JointReferential::copyConstructor(): Not Valid";
        return;
    }
    
    EntityItemPointer item = _tree->findEntityByID(_entityID);
    const Model* model = getModel(item);
    if (isValid() && model != NULL && _jointIndex < (uint32_t)(model->getJointStateCount())) {
        _lastRefDimension = item->getDimensions();
        model->getJointRotationInWorldFrame(_jointIndex, _refRotation);
        model->getJointPositionInWorldFrame(_jointIndex, _refPosition);
    }
    update();
}

JointReferential::JointReferential(uint32_t jointIndex, const QUuid& entityID, EntityTreePointer tree, AvatarData* avatar) :
    ModelReferential(entityID, tree, avatar),
    _jointIndex(jointIndex)
{
    _type = JOINT;
    EntityItemPointer item = _tree->findEntityByID(_entityID);
    const Model* model = getModel(item);
    if (!isValid() || model == NULL || _jointIndex >= (uint32_t)(model->getJointStateCount())) {
        qCDebug(interfaceapp) << "JointReferential::constructor(): Not Valid";
        _isValid = false;
        return;
    }
    
    _lastRefDimension = item->getDimensions();
    model->getJointRotationInWorldFrame(_jointIndex, _refRotation);
    model->getJointPositionInWorldFrame(_jointIndex, _refPosition);
    
    glm::quat refInvRot = glm::inverse(_refRotation);
    _rotation = refInvRot * _avatar->getOrientation();
    // BUG! _refPosition is in domain units, but avatar is in meters
    _translation = refInvRot * (avatar->getPosition() - _refPosition);
}

void JointReferential::update() {
    EntityItemPointer item = _tree->findEntityByID(_entityID);
    const Model* model = getModel(item);
    if (!isValid() || model == NULL || _jointIndex >= (uint32_t)(model->getJointStateCount())) {
        return;
    }
    
    bool somethingChanged = false;
    if (item->getDimensions() != _lastRefDimension) {
        glm::vec3 oldDimension = _lastRefDimension;
        _lastRefDimension = item->getDimensions();
        _translation *= _lastRefDimension / oldDimension;
        somethingChanged = true;
    }
    if (item->getRotation() != _refRotation) {
        model->getJointRotationInWorldFrame(_jointIndex, _refRotation);
        _avatar->setOrientation(_refRotation * _rotation, true);
        somethingChanged = true;
    }
    if (item->getPosition() != _refPosition || somethingChanged) {
        model->getJointPositionInWorldFrame(_jointIndex, _refPosition);
        _avatar->setPosition(_refPosition + _refRotation * _translation, true);
    }
}

const Model* JointReferential::getModel(EntityItemPointer item) {
    EntityItemFBXService* fbxService = _tree->getFBXService();
    if (item != NULL && fbxService != NULL) {
        return fbxService->getModelForEntityItem(item);
    }
    return NULL;
}

int JointReferential::packExtraData(unsigned char* destinationBuffer) const {
    unsigned char* startPosition = destinationBuffer;
    destinationBuffer += ModelReferential::packExtraData(destinationBuffer);
    
    memcpy(destinationBuffer, &_jointIndex, sizeof(_jointIndex));
    destinationBuffer += sizeof(_jointIndex);
    
    return destinationBuffer - startPosition;
}

int JointReferential::unpackExtraData(const unsigned char *sourceBuffer, int size) {
    const unsigned char* startPosition = sourceBuffer;
    sourceBuffer += ModelReferential::unpackExtraData(sourceBuffer, size);
    
    memcpy(&_jointIndex, sourceBuffer, sizeof(_jointIndex));
    sourceBuffer += sizeof(_jointIndex);
    
    return sourceBuffer - startPosition;
}
