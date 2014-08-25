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
#include "../renderer/Model.h"

#include "ModelReferential.h"

ModelReferential::ModelReferential(Referential* referential, EntityTree* tree, AvatarData* avatar) :
    Referential(MODEL, avatar),
    _tree(tree) 
{
    _translation = referential->getTranslation();
    _rotation = referential->getRotation();
    _scale = referential->getScale();
    unpackExtraData(reinterpret_cast<unsigned char*>(referential->getExtraData().data()),
                    referential->getExtraData().size());
    
    if (!isValid()) {
        qDebug() << "ModelReferential::copyConstructor(): Not Valid";
        return;
    }
    
    const EntityItem* item = _tree->findEntityByID(_entityID);
    if (item != NULL) {
        _refScale = item->getRadius();
        _refRotation = item->getRotation();
        _refPosition = item->getPosition() * (float)TREE_SCALE;
        update();
    }
}

ModelReferential::ModelReferential(const QUuid& entityID, EntityTree* tree, AvatarData* avatar) :
    Referential(MODEL, avatar),
    _entityID(entityID),
    _tree(tree)
{
    const EntityItem* item = _tree->findEntityByID(_entityID);
    if (!isValid() || item == NULL) {
        qDebug() << "ModelReferential::constructor(): Not Valid";
        _isValid = false;
        return;
    }
    
    _refScale = item->getRadius();
    _refRotation = item->getRotation();
    _refPosition = item->getPosition() * (float)TREE_SCALE;
    
    glm::quat refInvRot = glm::inverse(_refRotation);
    _scale = _avatar->getTargetScale() / _refScale;
    _rotation = refInvRot * _avatar->getOrientation();
    _translation = refInvRot * (avatar->getPosition() - _refPosition) / _refScale;
}

void ModelReferential::update() {
    const EntityItem* item = _tree->findEntityByID(_entityID);
    if (!isValid() || item == NULL || _avatar == NULL) {
        return;
    }
    
    bool somethingChanged = false;
    if (item->getRadius() != _refScale) {
        _refScale = item->getRadius();
        _avatar->setTargetScale(_refScale * _scale, true);
        somethingChanged = true;
    }
    if (item->getRotation() != _refRotation) {
        _refRotation = item->getRotation();
qDebug() << "ModelReferential::update() _refRotation=" << _refRotation;
        _avatar->setOrientation(_refRotation * _rotation, true);
        somethingChanged = true;
    }
    if (item->getPosition() != _refPosition || somethingChanged) {
        _refPosition = item->getPosition();
qDebug() << "ModelReferential::update() _refPosition=" << _refPosition << " QThread::currentThread()=" << QThread::currentThread();
        _avatar->setPosition(_refPosition * (float)TREE_SCALE + _refRotation * (_translation * _refScale), true);
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

JointReferential::JointReferential(Referential* referential, EntityTree* tree, AvatarData* avatar) :
    ModelReferential(referential, tree, avatar)
{
    _type = JOINT;
    if (!isValid()) {
        qDebug() << "JointReferential::copyConstructor(): Not Valid";
        return;
    }
    
    const EntityItem* item = _tree->findEntityByID(_entityID);
    const Model* model = getModel(item);
    if (!isValid() || model == NULL || _jointIndex >= model->getJointStateCount()) {
        _refScale = item->getRadius();
        model->getJointRotationInWorldFrame(_jointIndex, _refRotation);
        model->getJointPositionInWorldFrame(_jointIndex, _refPosition);
    }
    update();
}

JointReferential::JointReferential(uint32_t jointIndex, const QUuid& entityID, EntityTree* tree, AvatarData* avatar) :
    ModelReferential(entityID, tree, avatar),
    _jointIndex(jointIndex)
{
    _type = JOINT;
    const EntityItem* item = _tree->findEntityByID(_entityID);
    const Model* model = getModel(item);
    if (!isValid() || model == NULL || _jointIndex >= model->getJointStateCount()) {
        qDebug() << "JointReferential::constructor(): Not Valid";
        _isValid = false;
        return;
    }
    
    _refScale = item->getRadius();
    model->getJointRotationInWorldFrame(_jointIndex, _refRotation);
    model->getJointPositionInWorldFrame(_jointIndex, _refPosition);
    
    glm::quat refInvRot = glm::inverse(_refRotation);
    _scale = _avatar->getTargetScale() / _refScale;
    _rotation = refInvRot * _avatar->getOrientation();
    _translation = refInvRot * (avatar->getPosition() - _refPosition) / _refScale;
}

void JointReferential::update() {
    const EntityItem* item = _tree->findEntityByID(_entityID);
    const Model* model = getModel(item);
    if (!isValid() || model == NULL || _jointIndex >= model->getJointStateCount()) {
        return;
    }
    
    bool somethingChanged = false;
    if (item->getRadius() != _refScale) {
        _refScale = item->getRadius();
        _avatar->setTargetScale(_refScale * _scale, true);
        somethingChanged = true;
    }
    if (item->getRotation() != _refRotation) {
        model->getJointRotationInWorldFrame(_jointIndex, _refRotation);
        _avatar->setOrientation(_refRotation * _rotation, true);
        somethingChanged = true;
    }
    if (item->getPosition() != _refPosition || somethingChanged) {
        model->getJointPositionInWorldFrame(_jointIndex, _refPosition);
        _avatar->setPosition(_refPosition + _refRotation * (_translation * _refScale), true);
    }
}

const Model* JointReferential::getModel(const EntityItem* item) {
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