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
#include <Rig.h>
#include <ResourceManager.h>
#include <QDir>
#include <FSTReader.h>

const int NETWORKED_JOINTS_LIMIT = 256;
const float HIPS_GROUND_MIN_Y = 0.01f;
const float HIPS_SPINE_CHEST_MIN_SEPARATION = 0.001f;
const QString LEFT_JOINT_PREFIX = "Left";
const QString RIGHT_JOINT_PREFIX = "Right";

const QStringList LEG_MAPPING_SUFFIXES = {
    "UpLeg"
    "Leg",
    "Foot",
    "ToeBase",
};

static QStringList ARM_MAPPING_SUFFIXES = {
    "Shoulder",
    "Arm",
    "ForeArm",
    "Hand",
};

static QStringList HAND_MAPPING_SUFFIXES = {
    "HandIndex3",
    "HandIndex2",
    "HandIndex1",
    "HandPinky3",
    "HandPinky2",
    "HandPinky1",
    "HandMiddle3",
    "HandMiddle2",
    "HandMiddle1",
    "HandRing3",
    "HandRing2",
    "HandRing1",
    "HandThumb3",
    "HandThumb2",
    "HandThumb1",
};

const QUrl DEFAULT_URL = QUrl("https://docs.highfidelity.com/create/avatars/create-avatars.html#create-your-own-avatar");

AvatarDoctor::AvatarDoctor(const QUrl& avatarFSTFileUrl) :
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

    const auto resourceLoaded = [this, resource](bool success) {
        // MODEL
        if (!success) {
            _errors.push_back({ "Model file cannot be opened.", DEFAULT_URL });
            emit complete(getErrors());
            return;
        }
        _model = resource;
        const auto model = resource.data();
        const auto avatarModel = resource.data()->getHFMModel();
        if (!avatarModel.originalURL.endsWith(".fbx")) {
            _errors.push_back({ "Unsupported avatar model format.", DEFAULT_URL });
            emit complete(getErrors());
            return;
        }

        // RIG
        if (avatarModel.joints.isEmpty()) {
            _errors.push_back({ "Avatar has no rig.", DEFAULT_URL });
        } else {
            auto jointNames = avatarModel.getJointNames();

            if (avatarModel.joints.length() > NETWORKED_JOINTS_LIMIT) {
                _errors.push_back({tr( "Avatar has over %n bones.", "", NETWORKED_JOINTS_LIMIT), DEFAULT_URL });
            }
            // Avatar does not have Hips bone mapped	
            if (!jointNames.contains("Hips")) {
                _errors.push_back({ "Hips are not mapped.", DEFAULT_URL });
            }
            if (!jointNames.contains("Spine")) {
                _errors.push_back({ "Spine is not mapped.", DEFAULT_URL });
            }
            if (!jointNames.contains("Spine1")) {
                _errors.push_back({ "Chest (Spine1) is not mapped.", DEFAULT_URL });
            }
            if (!jointNames.contains("Neck")) {
                _errors.push_back({ "Neck is not mapped.", DEFAULT_URL });
            }
            if (!jointNames.contains("Head")) {
                _errors.push_back({ "Head is not mapped.", DEFAULT_URL });
            }

            if (!jointNames.contains("LeftEye")) {
                if (jointNames.contains("RightEye")) {
                    _errors.push_back({ "LeftEye is not mapped.", DEFAULT_URL });
                } else {
                    _errors.push_back({ "Eyes are not mapped.", DEFAULT_URL });
                }
            } else if (!jointNames.contains("RightEye")) {
                _errors.push_back({ "RightEye is not mapped.", DEFAULT_URL });
            }

            const auto checkJointAsymmetry = [jointNames] (const QStringList& jointMappingSuffixes) {
                foreach (const QString& jointSuffix, jointMappingSuffixes) {
                    if (jointNames.contains(LEFT_JOINT_PREFIX + jointSuffix) !=
                        jointNames.contains(RIGHT_JOINT_PREFIX + jointSuffix)) {
                        return true;
                    }
                }
                return false;
            };

            const auto isDescendantOfJointWhenJointsExist = [avatarModel, jointNames] (const QString& jointName, const QString& ancestorName) {
                if (!jointNames.contains(jointName) || !jointNames.contains(ancestorName)) {
                    return true;
                }
                auto currentJoint = avatarModel.joints[avatarModel.jointIndices[jointName] - 1];
                while (currentJoint.parentIndex != -1) {
                    currentJoint = avatarModel.joints[currentJoint.parentIndex];
                    if (currentJoint.name == ancestorName) {
                        return true;
                    }
                }
                return false;
            };

            if (checkJointAsymmetry(ARM_MAPPING_SUFFIXES)) {
                _errors.push_back({ "Asymmetrical arm bones.", DEFAULT_URL });
            }
            if (checkJointAsymmetry(HAND_MAPPING_SUFFIXES)) {
                _errors.push_back({ "Asymmetrical hand bones.", DEFAULT_URL });
            }
            if (checkJointAsymmetry(LEG_MAPPING_SUFFIXES)) {
                _errors.push_back({ "Asymmetrical leg bones.", DEFAULT_URL });
            }

            const auto rig = new Rig();
            rig->reset(avatarModel);
            const float eyeHeight = rig->getUnscaledEyeHeight();
            const float ratio = eyeHeight / DEFAULT_AVATAR_HEIGHT;
            const float avatarHeight = eyeHeight + ratio * DEFAULT_AVATAR_EYE_TO_TOP_OF_HEAD;
            qDebug() << "avatarHeight  = " << avatarHeight;

            // SCALE
            const float RECOMMENDED_MIN_HEIGHT = DEFAULT_AVATAR_HEIGHT * 0.25f;
            const float RECOMMENDED_MAX_HEIGHT = DEFAULT_AVATAR_HEIGHT * 1.5f;

            if (avatarHeight < RECOMMENDED_MIN_HEIGHT) {
                _errors.push_back({ "Avatar is possibly too short.", DEFAULT_URL });
            } else if (avatarHeight > RECOMMENDED_MAX_HEIGHT) {
                _errors.push_back({ "Avatar is possibly too tall.", DEFAULT_URL });
            }

            // HipsNotOnGround
            auto hipsIndex = rig->indexOfJoint("Hips");
            if (hipsIndex >= 0) {
                glm::vec3 hipsPosition;
                if (rig->getJointPosition(hipsIndex, hipsPosition)) {
                    const auto hipJoint = avatarModel.joints.at(avatarModel.getJointIndex("Hips"));

                    if (hipsPosition.y < HIPS_GROUND_MIN_Y) {
                        _errors.push_back({ "Hips are on ground.", DEFAULT_URL });
                    }
                }
            }

            // HipsSpineChestNotCoincident
            auto spineIndex = rig->indexOfJoint("Spine");
            auto chestIndex = rig->indexOfJoint("Spine1");
            if (hipsIndex >= 0 && spineIndex >= 0 && chestIndex >= 0) {
                glm::vec3 hipsPosition;
                glm::vec3 spinePosition;
                glm::vec3 chestPosition;
                if (rig->getJointPosition(hipsIndex, hipsPosition) &&
                    rig->getJointPosition(spineIndex, spinePosition) &&
                    rig->getJointPosition(chestIndex, chestPosition)) {

                    const auto hipsToSpine = glm::length(hipsPosition - spinePosition);
                    const auto spineToChest = glm::length(spinePosition - chestPosition);
                    if (hipsToSpine < HIPS_SPINE_CHEST_MIN_SEPARATION && spineToChest < HIPS_SPINE_CHEST_MIN_SEPARATION) {
                        _errors.push_back({ "Hips/Spine/Chest overlap.", DEFAULT_URL });
                    }
                }
            }
            rig->deleteLater();

            auto mapping = resource->getMapping();

            if (mapping.contains(JOINT_NAME_MAPPING_FIELD) && mapping[JOINT_NAME_MAPPING_FIELD].type() == QVariant::Hash) {
                const auto& jointNameMappings = mapping[JOINT_NAME_MAPPING_FIELD].toHash();
                QStringList jointValues;
                foreach(const auto& jointVariant, jointNameMappings.values()) {
                    jointValues << jointVariant.toString();
                }
                
                const auto& uniqueJointValues = jointValues.toSet();
                foreach (const auto& jointName, uniqueJointValues) {
                    if (jointValues.count(jointName) > 1) {
                        _errors.push_back({ tr("%1 is mapped multiple times.").arg(jointName), DEFAULT_URL });
                    }
                }
            }

            if (!isDescendantOfJointWhenJointsExist("Spine", "Hips")) {
                _errors.push_back({ "Spine is not a child of Hips.", DEFAULT_URL });
            }

            if (!isDescendantOfJointWhenJointsExist("Spine1", "Spine")) {
                _errors.push_back({ "Spine1 is not a child of Spine.", DEFAULT_URL });
            }

            if (!isDescendantOfJointWhenJointsExist("Head", "Spine1")) {
                _errors.push_back({ "Head is not a child of Spine1.", DEFAULT_URL });
            }
        }

        auto materialMappingHandled = [this]() mutable {
            _materialMappingLoadedCount++;
            // Continue diagnosing the textures as soon as the material mappings have tried to load.
            if (_materialMappingLoadedCount == _materialMappingCount) {
                // TEXTURES
                diagnoseTextures();
            }
        };

        _materialMappingCount = (int)model->getMaterialMapping().size();
        _materialMappingLoadedCount = 0;
        foreach(const auto& materialMapping, model->getMaterialMapping()) {
            // refresh the texture mappings
            auto materialMappingResource = materialMapping.second;
            if (materialMappingResource) {
                materialMappingResource->refresh();
                if (materialMappingResource->isLoaded()) {
                    materialMappingHandled();
                } else {
                    connect(materialMappingResource.data(), &NetworkTexture::finished, this,
                        [materialMappingHandled](bool success) mutable {
                        
                        materialMappingHandled();
                    });
                }
            } else {
                materialMappingHandled();
            }
        }
        if (_materialMappingCount == 0) {
            // TEXTURES
            diagnoseTextures();
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

void AvatarDoctor::diagnoseTextures() {
    const auto model = _model.data();
    const auto avatarModel = _model.data()->getHFMModel();
    QStringList externalTextures{};
    QSet<QString> textureNames{};
    int texturesFound = 0;
    auto addTextureToList = [&externalTextures, &texturesFound](hfm::Texture texture) mutable {
        if (texture.filename.isEmpty()) {
            return;
        }
        if (texture.content.isEmpty() && !externalTextures.contains(texture.name)) {
            externalTextures << texture.name;
        }
        texturesFound++;
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

    foreach(const auto& materialMapping, model->getMaterialMapping()) {
        foreach(const auto& networkMaterial, materialMapping.second.data()->parsedMaterials.networkMaterials) {
            texturesFound += (int)networkMaterial.second->getTextureMaps().size();
        }
    }

    auto normalizedURL = DependencyManager::get<ResourceManager>()->normalizeURL(
        QUrl(avatarModel.originalURL)).resolved(QUrl("textures"));

    if (texturesFound == 0) {
        _errors.push_back({ tr("No textures assigned."), DEFAULT_URL });
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
            auto checkTextureLoadingComplete = [this]() mutable {
                qDebug() << "checkTextureLoadingComplete" << _checkedTextureCount << "/" << _externalTextureCount;

                if (_checkedTextureCount == _externalTextureCount) {
                    if (_missingTextureCount > 0) {
                        _errors.push_back({ tr("Missing %n texture(s).","", _missingTextureCount), DEFAULT_URL });
                    }
                    if (_unsupportedTextureCount > 0) {
                        _errors.push_back({ tr("%n unsupported texture(s) found.", "", _unsupportedTextureCount),
                            DEFAULT_URL });
                    }

                    emit complete(getErrors());
                }
            };

            auto textureLoaded = [this, textureResource, checkTextureLoadingComplete](bool success) mutable {
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
