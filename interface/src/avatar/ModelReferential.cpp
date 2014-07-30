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

#include "ModelReferential.h"

ModelReferential::ModelReferential(uint32_t modelID, ModelTree* tree, AvatarData* avatar) :
Referential(avatar),
_modelID(modelID),
_tree(tree)
{
    const ModelItem* item = _tree->findModelByID(_modelID);
    if (!_isValid || item == NULL) {
        _isValid = false;
        return;
    }
    
    _refScale = item->getRadius();
    _refRotation = item->getModelRotation();
    _refPosition = item->getPosition();
    
    glm::quat refInvRot = glm::inverse(_refRotation);
    _scale = _avatar->getTargetScale() / _refScale;
    _rotation = refInvRot * _avatar->getOrientation();
    _translation = refInvRot * (avatar->getPosition() - _refPosition) / _refScale;
}

void ModelReferential::update() {
    const ModelItem* item = _tree->findModelByID(_modelID);
    if (!_isValid || item == NULL || _avatar == NULL) {
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
        _avatar->setPosition(_refPosition + _refRotation * (_translation * _refScale));
        somethingChanged = true;
    }
}

JointReferential::JointReferential(uint32_t jointID, uint32_t modelID, ModelTree* tree, AvatarData* avatar) :
    ModelReferential(modelID, tree, avatar),
    _jointID(jointID)
{
    const Model* model = getModel(_tree->findModelByID(_modelID));
    
    if (!_isValid || model == NULL || model->getJointStateCount() <= jointID) {
        _isValid = false;
        return;
    }
}

void JointReferential::update() {
    const ModelItem* item = _tree->findModelByID(_modelID);
    if (!_isValid || item == NULL) {
        _isValid = false;
        return;
    }
    
    
}

const Model* JointReferential::getModel(const ModelItem* item) {
    ModelItemFBXService* fbxService = _tree->getFBXService();
    if (item != NULL && fbxService != NULL) {
        return fbxService->getModelForModelItem(*item);
    }
    
    return NULL;
}
