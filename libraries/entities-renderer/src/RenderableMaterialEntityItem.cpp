//
//  Created by Sam Gondelman on 1/18/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableMaterialEntityItem.h"

#include "RenderPipelines.h"
#include "GeometryCache.h"

using namespace render;
using namespace render::entities;

bool MaterialEntityRenderer::needsRenderUpdate() const {
    if (_retryApply) {
        return true;
    }
    if (!_texturesLoaded) {
        return true;
    }
    return Parent::needsRenderUpdate();
}

bool MaterialEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    if (resultWithReadLock<bool>([&] {
        if (entity->getMaterialMappingMode() != _materialMappingMode) {
            return true;
        }
        if (entity->getMaterialRepeat() != _materialRepeat) {
            return true;
        }
        if (entity->getMaterialMappingPos() != _materialMappingPos || entity->getMaterialMappingScale() != _materialMappingScale || entity->getMaterialMappingRot() != _materialMappingRot) {
            return true;
        }
        if (entity->getTransform() != _transform) {
            return true;
        }
        if (entity->getUnscaledDimensions() != _dimensions) {
            return true;
        }

        if (entity->getMaterialURL() != _materialURL) {
            return true;
        }
        if (entity->getMaterialData() != _materialData) {
            return true;
        }
        if (entity->getParentMaterialName() != _parentMaterialName) {
            return true;
        }
        if (entity->getParentID() != _parentID) {
            return true;
        }
        if (entity->getPriority() != _priority) {
            return true;
        }

        return false;
    })) {
        return true;
    }
    return false;
}

void MaterialEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    withWriteLock([&] {
        bool deleteNeeded = false;
        bool addNeeded = _retryApply;
        bool transformChanged = false;
        {
            MaterialMappingMode mode = entity->getMaterialMappingMode();
            if (mode != _materialMappingMode) {
                _materialMappingMode = mode;
                transformChanged = true;
            }
        }
        {
            bool repeat = entity->getMaterialRepeat();
            if (repeat != _materialRepeat) {
                _materialRepeat = repeat;
                transformChanged = true;
            }
        }
        {
            glm::vec2 mappingPos = entity->getMaterialMappingPos();
            glm::vec2 mappingScale = entity->getMaterialMappingScale();
            float mappingRot = entity->getMaterialMappingRot();
            if (mappingPos != _materialMappingPos || mappingScale != _materialMappingScale || mappingRot != _materialMappingRot) {
                _materialMappingPos = mappingPos;
                _materialMappingScale = mappingScale;
                _materialMappingRot = mappingRot;
                transformChanged |= _materialMappingMode == MaterialMappingMode::UV;
            }
        }
        {
            Transform transform = entity->getTransform();
            glm::vec3 dimensions = entity->getUnscaledDimensions();
            if (transform != _transform || dimensions != _dimensions) {
                _transform = transform;
                _dimensions = dimensions;
                transformChanged |= _materialMappingMode == MaterialMappingMode::PROJECTED;
            }
        }

        {
            auto material = getMaterial();
            // Update the old material regardless of if it's going to change
            if (transformChanged && material && !_parentID.isNull()) {
                deleteNeeded = true;
                addNeeded = true;
                applyTextureTransform(material);
            }
        }

        bool urlChanged = false;
        std::string newCurrentMaterialName = _currentMaterialName;
        {
            QString materialURL = entity->getMaterialURL();
            if (materialURL != _materialURL) {
                _materialURL = materialURL;
                if (_materialURL.contains("#")) {
                    auto split = _materialURL.split("#");
                    newCurrentMaterialName = split.last().toStdString();
                } else if (_materialURL.contains("?")) {
                    qDebug() << "DEPRECATED: Use # instead of ? for material URLS:" << _materialURL;
                    auto split = _materialURL.split("?");
                    newCurrentMaterialName = split.last().toStdString();
                }
                urlChanged = true;
            }
        }

        bool usingMaterialData = _materialURL.startsWith("materialData");
        bool materialDataChanged = false;
        QUuid oldParentID = _parentID;
        QString oldParentMaterialName = _parentMaterialName;
        {
            QString materialData = entity->getMaterialData();
            if (materialData != _materialData) {
                _materialData = materialData;
                if (usingMaterialData) {
                    materialDataChanged = true;
                }
            }
        }
        {
            QString parentMaterialName = entity->getParentMaterialName();
            if (parentMaterialName != _parentMaterialName) {
                _parentMaterialName = parentMaterialName;
                deleteNeeded = true;
                addNeeded = true;
            }
        }
        {
            QUuid parentID = entity->getParentID();
            if (parentID != _parentID) {
                _parentID = parentID;
                deleteNeeded = true;
                addNeeded = true;
            }
        }
        {
            quint16 priority = entity->getPriority();
            if (priority != _priority) {
                _priority = priority;
                deleteNeeded = true;
                addNeeded = true;
            }
        }

        if (urlChanged && !usingMaterialData) {
            _networkMaterial = MaterialCache::instance().getMaterial(_materialURL);
            auto onMaterialRequestFinished = [this, oldParentID, oldParentMaterialName, newCurrentMaterialName](bool success) {
                if (success) {
                    deleteMaterial(oldParentID, oldParentMaterialName);
                    _texturesLoaded = false;
                    _parsedMaterials = _networkMaterial->parsedMaterials;
                    setCurrentMaterialName(newCurrentMaterialName);
                    applyMaterial();
                } else {
                    deleteMaterial(oldParentID, oldParentMaterialName);
                    _retryApply = false;
                    _texturesLoaded = true;
                }
            };
            if (_networkMaterial) {
                if (_networkMaterial->isLoaded()) {
                    onMaterialRequestFinished(!_networkMaterial->isFailed());
                } else {
                    connect(_networkMaterial.data(), &Resource::finished, this, [this, onMaterialRequestFinished](bool success) {
                        withWriteLock([&] {
                            onMaterialRequestFinished(success);
                        });
                    });
                }
            }
        } else if (materialDataChanged && usingMaterialData) {
            deleteMaterial(oldParentID, oldParentMaterialName);
            _texturesLoaded = false;
            _parsedMaterials = NetworkMaterialResource::parseJSONMaterials(QJsonDocument::fromJson(_materialData.toUtf8()), _materialURL);
            // Since our material changed, the current name might not be valid anymore, so we need to update
            setCurrentMaterialName(newCurrentMaterialName);
            applyMaterial();
        } else {
            if (deleteNeeded) {
                deleteMaterial(oldParentID, oldParentMaterialName);
            }
            if (addNeeded) {
                applyMaterial();
            }
        }

        {
            auto material = getMaterial();
            bool newTexturesLoaded = material ? !material->isMissingTexture() : false;
            if (!_texturesLoaded && newTexturesLoaded) {
                material->checkResetOpacityMap();
            }
            _texturesLoaded = newTexturesLoaded;
        }

        _renderTransform = getModelTransform();
        const float MATERIAL_ENTITY_SCALE = 0.5f;
        _renderTransform.postScale(MATERIAL_ENTITY_SCALE);
        _renderTransform.postScale(ENTITY_ITEM_DEFAULT_DIMENSIONS);
    });
}

ItemKey MaterialEntityRenderer::getKey() {
    ItemKey::Builder builder;
    builder.withTypeShape().withTagBits(getTagMask()).withLayer(getHifiRenderLayer());

    if (!_visible) {
        builder.withInvisible();
    }

    const auto drawMaterial = getMaterial();
    if (drawMaterial) {
        auto matKey = drawMaterial->getKey();
        if (matKey.isTranslucent()) {
            builder.withTransparent();
        }
    }

    return builder.build();
}

ShapeKey MaterialEntityRenderer::getShapeKey() {
    graphics::MaterialKey drawMaterialKey;
    const auto drawMaterial = getMaterial();
    if (drawMaterial) {
        drawMaterialKey = drawMaterial->getKey();
    }

    bool isTranslucent = drawMaterialKey.isTranslucent();
    bool hasTangents = drawMaterialKey.isNormalMap();
    bool hasLightmap = drawMaterialKey.isLightMap();
    bool isUnlit = drawMaterialKey.isUnlit();
    
    ShapeKey::Builder builder;
    builder.withMaterial();

    if (isTranslucent) {
        builder.withTranslucent();
    }
    if (hasTangents) {
        builder.withTangents();
    }
    if (hasLightmap) {
        builder.withLightMap();
    }
    if (isUnlit) {
        builder.withUnlit();
    }

    if (_primitiveMode == PrimitiveMode::LINES) {
        builder.withWireframe();
    }

    return builder.build();
}

void MaterialEntityRenderer::doRender(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableMaterialEntityItem::render");
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;

    // Don't render if our parent is set or our material is null
    QUuid parentID;
    withReadLock([&] {
        parentID = _parentID;
    });
    if (!parentID.isNull()) {
        return;
    }

    Transform renderTransform;
    graphics::MaterialPointer drawMaterial;
    Transform textureTransform;
    withReadLock([&] {
        renderTransform = _renderTransform;
        drawMaterial = getMaterial();
        textureTransform.setTranslation(glm::vec3(_materialMappingPos, 0));
        textureTransform.setRotation(glm::vec3(0, 0, glm::radians(_materialMappingRot)));
        textureTransform.setScale(glm::vec3(_materialMappingScale, 1));
    });
    if (!drawMaterial) {
        return;
    }

    batch.setModelTransform(renderTransform);

    drawMaterial->setTextureTransforms(textureTransform, MaterialMappingMode::UV, true);
    // bind the material
    if (RenderPipelines::bindMaterial(drawMaterial, batch, args->_renderMode, args->_enableTexturing)) {
        args->_details._materialSwitches++;
    }

    // Draw!
    DependencyManager::get<GeometryCache>()->renderSphere(batch);

    args->_details._trianglesRendered += (int)DependencyManager::get<GeometryCache>()->getSphereTriangleCount();
}

void MaterialEntityRenderer::setCurrentMaterialName(const std::string& currentMaterialName) {
    if (_parsedMaterials.networkMaterials.find(currentMaterialName) != _parsedMaterials.networkMaterials.end()) {
        _currentMaterialName = currentMaterialName;
    } else if (_parsedMaterials.names.size() > 0) {
        _currentMaterialName = _parsedMaterials.names[0];
    }
}

std::shared_ptr<NetworkMaterial> MaterialEntityRenderer::getMaterial() const {
    auto material = _parsedMaterials.networkMaterials.find(_currentMaterialName);
    if (material != _parsedMaterials.networkMaterials.end()) {
        return material->second;
    } else {
        return nullptr;
    }
}

void MaterialEntityRenderer::deleteMaterial(const QUuid& oldParentID, const QString& oldParentMaterialName) {
    std::shared_ptr<NetworkMaterial> material = _appliedMaterial;
    if (!material || oldParentID.isNull()) {
        return;
    }

    // Our parent could be an entity or an avatar
    std::string oldParentMaterialNameStd = oldParentMaterialName.toStdString();
    if (EntityTreeRenderer::removeMaterialFromEntity(oldParentID, material, oldParentMaterialNameStd)) {
        _appliedMaterial = nullptr;
        return;
    }

    if (EntityTreeRenderer::removeMaterialFromAvatar(oldParentID, material, oldParentMaterialNameStd)) {
        _appliedMaterial = nullptr;
        return;
    }

    // if a remove fails, our parent is gone, so we don't need to retry, EXCEPT:
    // MyAvatar can change UUIDs when you switch domains, which leads to a timing issue.  Let's just make
    // sure we weren't attached to MyAvatar by trying this (if we weren't, this will have no effect)
    if (EntityTreeRenderer::removeMaterialFromAvatar(AVATAR_SELF_ID, material, oldParentMaterialNameStd)) {
        _appliedMaterial = nullptr;
        return;
    }
}

void MaterialEntityRenderer::applyTextureTransform(std::shared_ptr<NetworkMaterial>& material) {
    Transform textureTransform;
    if (_materialMappingMode == MaterialMappingMode::UV) {
        textureTransform.setTranslation(glm::vec3(_materialMappingPos, 0.0f));
        textureTransform.setRotation(glm::vec3(0.0f, 0.0f, glm::radians(_materialMappingRot)));
        textureTransform.setScale(glm::vec3(_materialMappingScale, 1.0f));
    } else if (_materialMappingMode == MaterialMappingMode::PROJECTED) {
        textureTransform = _transform;
        textureTransform.postScale(_dimensions);
        // Pass the inverse transform here so we don't need to compute it in the shaders
        textureTransform.evalFromRawMatrix(textureTransform.getInverseMatrix());
    }
    material->setTextureTransforms(textureTransform, _materialMappingMode, _materialRepeat);
}

void MaterialEntityRenderer::applyMaterial() {
    _retryApply = false;

    std::shared_ptr<NetworkMaterial> material = getMaterial();
    QUuid parentID = _parentID;
    if (!material || parentID.isNull()) {
        _appliedMaterial = nullptr;
        return;
    }

    applyTextureTransform(material);

    graphics::MaterialLayer materialLayer = graphics::MaterialLayer(material, _priority);

    // Our parent could be an entity or an avatar
    std::string parentMaterialName = _parentMaterialName.toStdString();
    if (EntityTreeRenderer::addMaterialToEntity(parentID, materialLayer, parentMaterialName)) {
        _appliedMaterial = material;
        return;
    }

    if (EntityTreeRenderer::addMaterialToAvatar(parentID, materialLayer, parentMaterialName)) {
        _appliedMaterial = material;
        return;
    }

    // if we've reached this point, we couldn't find our parent, so we need to try again later
    _retryApply = true;
}