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
    // Could require material re-apply
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

    // Won't cause material re-apply
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
    return false;
}

void MaterialEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    withWriteLock([&] {
        if (_drawMaterial != entity->getMaterial()) {
            _texturesLoaded = false;
            _drawMaterial = entity->getMaterial();
        }
        _parentID = entity->getParentID();
        _materialMappingPos = entity->getMaterialMappingPos();
        _materialMappingScale = entity->getMaterialMappingScale();
        _materialMappingRot = entity->getMaterialMappingRot();
        _renderTransform = getModelTransform();
        const float MATERIAL_ENTITY_SCALE = 0.5f;
        _renderTransform.postScale(MATERIAL_ENTITY_SCALE);
        _renderTransform.postScale(ENTITY_ITEM_DEFAULT_DIMENSIONS);

        bool newTexturesLoaded = _drawMaterial ? !_drawMaterial->isMissingTexture() : false;
        if (!_texturesLoaded && newTexturesLoaded) {
            _drawMaterial->checkResetOpacityMap();
        }
        _texturesLoaded = newTexturesLoaded;
    });
}

ItemKey MaterialEntityRenderer::getKey() {
    ItemKey::Builder builder;
    builder.withTypeShape().withTagBits(getTagMask()).withLayer(getHifiRenderLayer());

    if (!_visible) {
        builder.withInvisible();
    }

    const auto& drawMaterial = getMaterial();
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
    const auto& drawMaterial = getMaterial();
    if (drawMaterial) {
        drawMaterialKey = drawMaterial->getKey();
    }

    bool isTranslucent = drawMaterialKey.isTranslucent();
    bool hasTangents = drawMaterialKey.isNormalMap();
    bool hasLightmap = drawMaterialKey.isLightmapMap();
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
        builder.withLightmap();
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

    if (args->_renderMode != render::Args::RenderMode::SHADOW_RENDER_MODE) {
        drawMaterial->setTextureTransforms(textureTransform, MaterialMappingMode::UV, true);

        // bind the material
        RenderPipelines::bindMaterial(drawMaterial, batch, args->_enableTexturing);
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

void MaterialEntityRenderer::deleteMaterial() {
    std::shared_ptr<NetworkMaterial> material = getMaterial();
    if (!material) {
        return;
    }
    QUuid parentID = _parentID;
    if (parentID.isNull()) {
        return;
    }

    // Our parent could be an entity or an avatar
    if (EntityTreeRenderer::removeMaterialFromEntity(parentID, material, _parentMaterialName.toStdString())) {
        return;
    }

    if (EntityTreeRenderer::removeMaterialFromAvatar(parentID, material, _parentMaterialName.toStdString())) {
        return;
    }

    // if a remove fails, our parent is gone, so we don't need to retry
}

void MaterialEntityRenderer::applyMaterial() {
    _retryApply = false;
    std::shared_ptr<NetworkMaterial> material = getMaterial();
    QUuid parentID = _parentID;
    if (!material || parentID.isNull()) {
        return;
    }

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

    graphics::MaterialLayer materialLayer = graphics::MaterialLayer(material, _priority);

    // Our parent could be an entity or an avatar
    if (EntityTreeRenderer::addMaterialToEntity(parentID, materialLayer, _parentMaterialName.toStdString())) {
        return;
    }

    if (EntityTreeRenderer::addMaterialToAvatar(parentID, materialLayer, _parentMaterialName.toStdString())) {
        return;
    }

    // if we've reached this point, we couldn't find our parent, so we need to try again later
    _retryApply = true;
}