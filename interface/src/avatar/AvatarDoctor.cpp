//
//  AvatarDoctor.cpp
//
//
//  Created by Thijs Wenker on 2/12/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarDoctor.h"
#include <model-networking/ModelCache.h>

AvatarDoctor::AvatarDoctor(QUrl avatarFSTFileUrl) :
    _avatarFSTFileUrl(std::move(avatarFSTFileUrl)) {
}

void AvatarDoctor::startDiagnosing() {
    _errors.clear();
    const auto resource = DependencyManager::get<ModelCache>()->getGeometryResource(_avatarFSTFileUrl);
    const auto resourceLoaded = [this, resource](bool success) {
        // MODEL
        if (!success) {
            _errors.push_back({ "Model file cannot be opened", QUrl("http://www.highfidelity.com/docs") });
            emit complete(getErrors());
            return;
        }
        const auto avatarModel = resource.data()->getHFMModel();
        if (!avatarModel.originalURL.endsWith(".fbx")) {
            _errors.push_back({ "Unsupported avatar model format", QUrl("http://www.highfidelity.com/docs") });
            emit complete(getErrors());
            return;
        }

        // RIG
        if (avatarModel.joints.isEmpty()) {
            _errors.push_back({ "Avatar has no rig", QUrl("http://www.highfidelity.com/docs") });
        }
        else {
            if (avatarModel.joints.length() > 256) {
                _errors.push_back({ "Avatar has over 256 bones", QUrl("http://www.highfidelity.com/docs") });
            }
            // Avatar does not have Hips bone mapped	
            if (!avatarModel.getJointNames().contains("Hips")) {
                _errors.push_back({ "Hips are not mapped", QUrl("http://www.highfidelity.com/docs") });
            }
            if (!avatarModel.getJointNames().contains("Spine")) {
                _errors.push_back({ "Spine is not mapped", QUrl("http://www.highfidelity.com/docs") });
            }
            if (!avatarModel.getJointNames().contains("Head")) {
                _errors.push_back({ "Head is not mapped", QUrl("http://www.highfidelity.com/docs") });
            }
        }

        // SCALE
        const float DEFAULT_HEIGHT = 1.75f;
        const float RECOMMENDED_MIN_HEIGHT = DEFAULT_HEIGHT * 0.25;
        const float RECOMMENDED_MAX_HEIGHT = DEFAULT_HEIGHT * 1.5;

        float avatarHeight = avatarModel.getMeshExtents().largestDimension();

        qWarning() << "avatarHeight" << avatarHeight;
        if (avatarHeight < RECOMMENDED_MIN_HEIGHT) {
            _errors.push_back({ "Avatar is possibly smaller then expected.", QUrl("http://www.highfidelity.com/docs") });
        }
        else if (avatarHeight > RECOMMENDED_MAX_HEIGHT) {
            _errors.push_back({ "Avatar is possibly larger then expected.", QUrl("http://www.highfidelity.com/docs") });
        }

        // BLENDSHAPES

        // TEXTURES
        //avatarModel.materials.


        emit complete(getErrors());
    };

    if (resource) {
        if (resource->isLoaded()) {
            resourceLoaded(!resource->isFailed());
        } else {
            connect(resource.data(), &GeometryResource::finished, this, resourceLoaded);
        }
    } else {
        _errors.push_back({ "Model file cannot be opened", QUrl("http://www.highfidelity.com/docs") });
        emit complete(getErrors());
    }    
}

QVariantList AvatarDoctor::getErrors() const {
    QVariantList result;
    for (const auto& error : _errors) {
        QVariantMap errorVariant;
        errorVariant.insert("message", error.message);
        errorVariant.insert("url", error.url);
        result.append(errorVariant);
    }
    return result;
}
