//
//  AvatarDoctor.cpp
//
//  Created by Thijs Wenker on 2/12/2019.
//  Copyright 2019 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
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

const QUrl PACKAGE_AVATAR_DOCS_BASE_URL = QUrl("https://docs.vircadia.com/create/avatars/package-avatar.html");

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
            addError("Model file cannot be opened.", "missing-file");
            emit complete(getErrors());
            return;
        }
        _model = resource;
        const auto model = resource.data();
        const auto avatarModel = resource.data()->getHFMModel();
        if (!avatarModel.originalURL.toLower().endsWith(".fbx")) {
            addError("Unsupported avatar model format.", "unsupported-format");
            emit complete(getErrors());
            return;
        }

        // RIG
        if (avatarModel.joints.isEmpty()) {
            addError("Avatar has no rig.", "no-rig");
        } else {
            auto jointNames = avatarModel.getJointNames();

            if (avatarModel.joints.length() > NETWORKED_JOINTS_LIMIT) {
                addError(tr( "Avatar has over %n bones.", "", NETWORKED_JOINTS_LIMIT), "maximum-bone-limit");
            }
            // Avatar does not have Hips bone mapped
            if (!jointNames.contains("Hips")) {
                addError("Hips are not mapped.", "hips-not-mapped");
            }
            if (!jointNames.contains("Spine")) {
                addError("Spine is not mapped.", "spine-not-mapped");
            }
            if (!jointNames.contains("Spine1")) {
                addError("Chest (Spine1) is not mapped.", "chest-not-mapped");
            }
            if (!jointNames.contains("Neck")) {
                addError("Neck is not mapped.", "neck-not-mapped");
            }
            if (!jointNames.contains("Head")) {
                addError("Head is not mapped.", "head-not-mapped");
            }

            if (!jointNames.contains("LeftEye")) {
                if (jointNames.contains("RightEye")) {
                    addError("LeftEye is not mapped.", "eye-not-mapped");
                } else {
                addError("Eyes are not mapped.", "eye-not-mapped");
                }
            } else if (!jointNames.contains("RightEye")) {
                addError("RightEye is not mapped.", "eye-not-mapped");
            }

            const auto checkJointAsymmetry = [jointNames] (const QStringList& jointMappingSuffixes) {
                for (const QString& jointSuffix : jointMappingSuffixes) {
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
                addError("Asymmetrical arm bones.", "asymmetrical-bones");
            }
            if (checkJointAsymmetry(HAND_MAPPING_SUFFIXES)) {
                addError("Asymmetrical hand bones.", "asymmetrical-bones");
            }
            if (checkJointAsymmetry(LEG_MAPPING_SUFFIXES)) {
                addError("Asymmetrical leg bones.", "asymmetrical-bones");
            }

            // Multiple skeleton root joints checkup
            int skeletonRootJoints = 0;
            for (const HFMJoint& joint: avatarModel.joints) {
                if (joint.parentIndex == -1 && joint.isSkeletonJoint) {
                    skeletonRootJoints++;
                }
            }

            if (skeletonRootJoints > 1) {
                addError("Multiple top-level joints found.", "multiple-top-level-joints");
            }

            Rig rig;
            rig.reset(avatarModel);
            const float eyeHeight = rig.getUnscaledEyeHeight();
            const float ratio = eyeHeight / DEFAULT_AVATAR_HEIGHT;
            const float avatarHeight = eyeHeight + ratio * DEFAULT_AVATAR_EYE_TO_TOP_OF_HEAD;

            // SCALE
            const float RECOMMENDED_MIN_HEIGHT = DEFAULT_AVATAR_HEIGHT * 0.25f;
            const float RECOMMENDED_MAX_HEIGHT = DEFAULT_AVATAR_HEIGHT * 1.5f;

            if (avatarHeight < RECOMMENDED_MIN_HEIGHT) {
                addError("Avatar is possibly too short.", "short-avatar");
            } else if (avatarHeight > RECOMMENDED_MAX_HEIGHT) {
                addError("Avatar is possibly too tall.", "tall-avatar");
            }

            // HipsNotOnGround
            auto hipsIndex = rig.indexOfJoint("Hips");
            if (hipsIndex >= 0) {
                glm::vec3 hipsPosition;
                if (rig.getJointPosition(hipsIndex, hipsPosition)) {
                    const auto hipJoint = avatarModel.joints.at(avatarModel.getJointIndex("Hips"));

                    if (hipsPosition.y < HIPS_GROUND_MIN_Y) {
                        addError("Hips are on ground.", "hips-on-ground");
                    }
                }
            }

            // HipsSpineChestNotCoincident
            auto spineIndex = rig.indexOfJoint("Spine");
            auto chestIndex = rig.indexOfJoint("Spine1");
            if (hipsIndex >= 0 && spineIndex >= 0 && chestIndex >= 0) {
                glm::vec3 hipsPosition;
                glm::vec3 spinePosition;
                glm::vec3 chestPosition;
                if (rig.getJointPosition(hipsIndex, hipsPosition) &&
                    rig.getJointPosition(spineIndex, spinePosition) &&
                    rig.getJointPosition(chestIndex, chestPosition)) {

                    const auto hipsToSpine = glm::length(hipsPosition - spinePosition);
                    const auto spineToChest = glm::length(spinePosition - chestPosition);
                    if (hipsToSpine < HIPS_SPINE_CHEST_MIN_SEPARATION && spineToChest < HIPS_SPINE_CHEST_MIN_SEPARATION) {
                        addError("Hips/Spine/Chest overlap.", "overlap-error");
                    }
                }
            }

            auto mapping = resource->getMapping();

            if (mapping.contains(JOINT_NAME_MAPPING_FIELD) && mapping[JOINT_NAME_MAPPING_FIELD].type() == QVariant::Hash) {
                const auto& jointNameMappings = mapping[JOINT_NAME_MAPPING_FIELD].toHash();
                QStringList jointValues;
                for (const auto& jointVariant: jointNameMappings.values()) {
                    jointValues << jointVariant.toString();
                }

                const auto& uniqueJointValues = jointValues.toSet();
                for (const auto& jointName: uniqueJointValues) {
                    if (jointValues.count(jointName) > 1) {
                        addError(tr("%1 is mapped multiple times.").arg(jointName), "mapped-multiple-times");
                    }
                }
            }

            if (!isDescendantOfJointWhenJointsExist("Spine", "Hips")) {
                addError("Spine is not a child of Hips.", "spine-not-child");
            }

            if (!isDescendantOfJointWhenJointsExist("Spine1", "Spine")) {
                addError("Spine1 is not a child of Spine.", "spine1-not-child");
            }

            if (!isDescendantOfJointWhenJointsExist("Head", "Spine1")) {
                addError("Head is not a child of Spine1.", "head-not-child");
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
        for (const auto& materialMapping : model->getMaterialMapping()) {
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
        addError("Model file cannot be opened", "missing-file");
        emit complete(getErrors());
    }
}

void AvatarDoctor::diagnoseTextures() {
    const auto model = _model.data();
    const auto avatarModel = _model.data()->getHFMModel();
    QVector<QString> externalTextures{};
    QVector<QString> textureNames{};
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

    for (const auto& material : avatarModel.materials) {
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

    for (const auto& materialMapping : model->getMaterialMapping()) {
        for (const auto& networkMaterial : materialMapping.second.data()->parsedMaterials.networkMaterials) {
            texturesFound += (int)networkMaterial.second->getTextureMaps().size();
        }
    }

    auto normalizedURL = DependencyManager::get<ResourceManager>()->normalizeURL(
        QUrl(avatarModel.originalURL)).resolved(QUrl("textures"));

    if (texturesFound == 0) {
        addError(tr("No textures assigned."), "no-textures-assigned");
    }

    if (!externalTextures.empty()) {
        // Check External Textures:
        auto modelTexturesURLs = model->getTextures();
        _externalTextureCount = externalTextures.length();

        auto checkTextureLoadingComplete = [this]() mutable {
            if (_checkedTextureCount == _externalTextureCount) {
                if (_missingTextureCount > 0) {
                    addError(tr("Missing %n texture(s).","", _missingTextureCount), "missing-textures");
                }
                if (_unsupportedTextureCount > 0) {
                    addError(tr("%n unsupported texture(s) found.", "", _unsupportedTextureCount), "unsupported-textures");
                }

                emit complete(getErrors());
            }
        };

        for (const QString& textureKey : externalTextures) {
            if (!modelTexturesURLs.contains(textureKey)) {
                _missingTextureCount++;
                _checkedTextureCount++;
                continue;
            }
            const QUrl textureURL = modelTexturesURLs[textureKey].toUrl();
            auto textureResource = DependencyManager::get<TextureCache>()->getTexture(textureURL);
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
            }
        }
        checkTextureLoadingComplete();
    } else {
        emit complete(getErrors());
    }
}

void AvatarDoctor::addError(const QString& errorMessage, const QString& docFragment) {
    QUrl documentationURL = PACKAGE_AVATAR_DOCS_BASE_URL;
    documentationURL.setFragment(docFragment);
    _errors.push_back({ errorMessage, documentationURL });
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
