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
#include <AvatarConstants.h>
#include <ResourceManager.h>


AvatarDoctor::AvatarDoctor(QUrl avatarFSTFileUrl) :
    _avatarFSTFileUrl(avatarFSTFileUrl) {

    connect(this, &AvatarDoctor::complete, this, [this](QVariantList errors) {
        _isDiagnosing = false;
    });
}

void AvatarDoctor::startDiagnosing() {
    if (_isDiagnosing) {
        // One diagnose at a time for now
        return;
    }
    _isDiagnosing = true;
    
    _errors.clear();

    _externalTextureCount = 0;
    _checkedTextureCount = 0;
    _missingTextureCount = 0;
    _unsupportedTextureCount = 0;

    const auto resource = DependencyManager::get<ModelCache>()->getGeometryResource(_avatarFSTFileUrl);
    resource->refresh();
    const QUrl DEFAULT_URL = QUrl("https://docs.highfidelity.com/create/avatars/create-avatars.html#create-your-own-avatar");
    const auto resourceLoaded = [this, resource, DEFAULT_URL](bool success) {
        // MODEL
        if (!success) {
            _errors.push_back({ "Model file cannot be opened", DEFAULT_URL });
            emit complete(getErrors());
            return;
        }
        const auto model = resource.data();
        const auto avatarModel = resource.data()->getHFMModel();
        if (!avatarModel.originalURL.endsWith(".fbx")) {
            _errors.push_back({ "Unsupported avatar model format", DEFAULT_URL });
            emit complete(getErrors());
            return;
        }

        // RIG
        if (avatarModel.joints.isEmpty()) {
            _errors.push_back({ "Avatar has no rig", DEFAULT_URL });
        } else {
            if (avatarModel.joints.length() > 256) {
                _errors.push_back({ "Avatar has over 256 bones", DEFAULT_URL });
            }
            // Avatar does not have Hips bone mapped	
            if (!avatarModel.getJointNames().contains("Hips")) {
                _errors.push_back({ "Hips are not mapped", DEFAULT_URL });
            }
            if (!avatarModel.getJointNames().contains("Spine")) {
                _errors.push_back({ "Spine is not mapped", DEFAULT_URL });
            }
            if (!avatarModel.getJointNames().contains("Head")) {
                _errors.push_back({ "Head is not mapped", DEFAULT_URL });
            }
        }

        // SCALE
        const float RECOMMENDED_MIN_HEIGHT = DEFAULT_AVATAR_HEIGHT * 0.25f;
        const float RECOMMENDED_MAX_HEIGHT = DEFAULT_AVATAR_HEIGHT * 1.5f;
        
        const float avatarHeight = avatarModel.bindExtents.largestDimension();
        if (avatarHeight < RECOMMENDED_MIN_HEIGHT) {
            _errors.push_back({ "Avatar is possibly too small.", DEFAULT_URL });
        } else if (avatarHeight > RECOMMENDED_MAX_HEIGHT) {
            _errors.push_back({ "Avatar is possibly too large.", DEFAULT_URL });
        }

        // TEXTURES
        QStringList externalTextures{};
        QSet<QString> textureNames{};
        auto addTextureToList = [&externalTextures](hfm::Texture texture) mutable {
            if (!texture.filename.isEmpty() && texture.content.isEmpty() && !externalTextures.contains(texture.name)) {
                externalTextures << texture.name;
            }
        };
        
        foreach(const HFMMaterial material, avatarModel.materials) {
            addTextureToList(material.normalTexture);
            addTextureToList(material.albedoTexture);
            addTextureToList(material.opacityTexture);
            addTextureToList(material.glossTexture);
            addTextureToList(material.roughnessTexture);
            addTextureToList(material.specularTexture);
            addTextureToList(material.metallicTexture);
            addTextureToList(material.emissiveTexture);
            addTextureToList(material.occlusionTexture);
            addTextureToList(material.scatteringTexture);
            addTextureToList(material.lightmapTexture);
        }

        if (!externalTextures.empty()) {
            // Check External Textures:
            auto modelTexturesURLs = model->getTextures();
            _externalTextureCount = externalTextures.length();
            foreach(const QString textureKey, externalTextures) {
                if (!modelTexturesURLs.contains(textureKey)) {
                    _missingTextureCount++;
                    _checkedTextureCount++;
                    continue;
                }

                const QUrl textureURL = modelTexturesURLs[textureKey].toUrl();

                auto textureResource = DependencyManager::get<TextureCache>()->getTexture(textureURL);
                auto checkTextureLoadingComplete = [this, DEFAULT_URL] () mutable {
                    qDebug() << "checkTextureLoadingComplete" << _checkedTextureCount << "/" << _externalTextureCount;

                    if (_checkedTextureCount == _externalTextureCount) {
                        if (_missingTextureCount > 0) {
                            _errors.push_back({ tr("Missing %n texture(s).","", _missingTextureCount), DEFAULT_URL });
                        }
                        if (_unsupportedTextureCount > 0) {
                            _errors.push_back({ tr("%n unsupported texture(s) found.", "", _unsupportedTextureCount), DEFAULT_URL });
                        }
                        emit complete(getErrors());
                    }
                };

                auto textureLoaded = [this, textureResource, checkTextureLoadingComplete] (bool success) mutable {
                    if (!success) {
                        auto normalizedURL = DependencyManager::get<ResourceManager>()->normalizeURL(textureResource->getURL());
                        if (normalizedURL.isLocalFile()) {
                            QFile textureFile(normalizedURL.toLocalFile());
                            if (textureFile.exists()) {
                                _unsupportedTextureCount++;
                            } else {
                                _missingTextureCount++;
                            }
                        } else {
                            _missingTextureCount++;
                        }
                    }
                    _checkedTextureCount++;
                    checkTextureLoadingComplete();
                };

                if (textureResource) {
                    textureResource->refresh();
                    if (textureResource->isLoaded()) {
                        textureLoaded(!textureResource->isFailed());
                    } else {
                        connect(textureResource.data(), &NetworkTexture::finished, this, textureLoaded);
                    }
                } else {
                    _missingTextureCount++;
                    _checkedTextureCount++;
                    checkTextureLoadingComplete();
                }
            }
        } else {
            emit complete(getErrors());
        }
    };

    if (resource) {
        if (resource->isLoaded()) {
            resourceLoaded(!resource->isFailed());
        } else {
            connect(resource.data(), &GeometryResource::finished, this, resourceLoaded);
        }
    } else {
        _errors.push_back({ "Model file cannot be opened", DEFAULT_URL });
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
