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

#include "ModelTree.h"
#include "../renderer/Model.h"

#include "ModelReferential.h"

ModelReferential::ModelReferential(Referential* referential, AvatarData* avatar) : Referential(MODEL, avatar) {
    
}

ModelReferential::ModelReferential(uint32_t modelID, ModelTree* tree, AvatarData* avatar) :
Referential(MODEL, avatar),
_modelID(modelID),
_tree(tree)
{
    const ModelItem* item = _tree->findModelByID(_modelID);
    if (!_isValid || item == NULL) {
        qDebug() << "Not Validd";
        _isValid = false;
        return;
    }
    
    _refScale = item->getRadius();
    _refRotation = item->getModelRotation();
    _refPosition = item->getPosition() * (float)TREE_SCALE;
    
    glm::quat refInvRot = glm::inverse(_refRotation);
    _scale = _avatar->getTargetScale() / _refScale;
    _rotation = refInvRot * _avatar->getOrientation();
    _translation = refInvRot * (avatar->getPosition() - _refPosition) / _refScale;
}

void ModelReferential::update() {
    const ModelItem* item = _tree->findModelByID(_modelID);
    if (item == NULL || _avatar == NULL) {
        qDebug() << "Not Valid";
        _isValid = false;
        return;
    }
    
    bool somethingChanged = false;
    if (item->getRadius() != _refScale) {
        _refScale = item->getRadius();
        _avatar->setTargetScale(_refScale * _scale);
        somethingChanged = true;
    }
    if (item->getModelRotation() != _refRotation) {
        _refRotation = item->getModelRotation();
        _avatar->setOrientation(_refRotation * _rotation);
        somethingChanged = true;
    }
    if (item->getPosition() != _refPosition || somethingChanged) {
        _refPosition = item->getPosition();
        _avatar->setPosition(_refPosition * (float)TREE_SCALE + _refRotation * (_translation * _refScale));
        //qDebug() << "Ref: " << item->getLastUpdated() << " " << item->getLastEdited();
        somethingChanged = true;
    }
}

int ModelReferential::packExtraData(unsigned char* destinationBuffer) {
    memcpy(destinationBuffer, &_modelID, sizeof(_modelID));
    return sizeof(_modelID);
}

int ModelReferential::unpackExtraData(const unsigned char *sourceBuffer) {
    memcpy(&_modelID, sourceBuffer, sizeof(_modelID));
    return sizeof(_modelID);
}

JointReferential::JointReferential(uint32_t jointIndex, uint32_t modelID, ModelTree* tree, AvatarData* avatar) :
    ModelReferential(modelID, tree, avatar),
    _jointIndex(jointIndex)
{
    _type = JOINT;
    const ModelItem* item = _tree->findModelByID(_modelID);
    const Model* model = getModel(item);
    if (!_isValid || model == NULL || _jointIndex >= model->getJointStateCount()) {
        qDebug() << "Not Validd";
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
    const ModelItem* item = _tree->findModelByID(_modelID);
    const Model* model = getModel(item);
    if (model == NULL || _jointIndex >= model->getJointStateCount()) {
        qDebug() << "Not Valid";
        _isValid = false;
        return;
    }
    
    bool somethingChanged = false;
    if (item->getRadius() != _refScale) {
        _refScale = item->getRadius();
        _avatar->setTargetScale(_refScale * _scale);
        somethingChanged = true;
    }
    if (item->getModelRotation() != _refRotation) {
        model->getJointRotationInWorldFrame(_jointIndex, _refRotation);
        _avatar->setOrientation(_refRotation * _rotation);
        somethingChanged = true;
    }
    if (item->getPosition() != _refPosition || somethingChanged) {
        model->getJointPositionInWorldFrame(_jointIndex, _refPosition);
        _avatar->setPosition(_refPosition + _refRotation * (_translation * _refScale));
        somethingChanged = true;
    }
}

const Model* JointReferential::getModel(const ModelItem* item) {
    ModelItemFBXService* fbxService = _tree->getFBXService();
    if (item != NULL && fbxService != NULL) {
        return fbxService->getModelForModelItem(*item);
    }
    qDebug() << "No Model";
    
    return NULL;
}
