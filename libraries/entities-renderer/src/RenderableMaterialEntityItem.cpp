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

bool MaterialEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    if (entity->getMaterial() != _drawMaterial) {
        return true;
    }
    if (entity->getParentID() != _parentID) {
        return true;
    }
    if (entity->getMaterialMappingPos() != _materialMappingPos || entity->getMaterialMappingScale() != _materialMappingScale || entity->getMaterialMappingRot() != _materialMappingRot) {
        return true;
    }
    return false;
}

void MaterialEntityRenderer::doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) {
    withWriteLock([&] {
        _drawMaterial = entity->getMaterial();
        _parentID = entity->getParentID();
        _materialMappingPos = entity->getMaterialMappingPos();
        _materialMappingScale = entity->getMaterialMappingScale();
        _materialMappingRot = entity->getMaterialMappingRot();
        _renderTransform = getModelTransform();
        const float MATERIAL_ENTITY_SCALE = 0.5f;
        _renderTransform.postScale(MATERIAL_ENTITY_SCALE);
        _renderTransform.postScale(ENTITY_ITEM_DEFAULT_DIMENSIONS);
    });
}

ItemKey MaterialEntityRenderer::getKey() {
    ItemKey::Builder builder;
    builder.withTypeShape().withTagBits(getTagMask());

    if (!_visible) {
        builder.withInvisible();
    }

    if (_drawMaterial) {
        auto matKey = _drawMaterial->getKey();
        if (matKey.isTranslucent()) {
            builder.withTransparent();
        }
    }

    return builder.build();
}

ShapeKey MaterialEntityRenderer::getShapeKey() {
    graphics::MaterialKey drawMaterialKey;
    if (_drawMaterial) {
        drawMaterialKey = _drawMaterial->getKey();
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

    return builder.build();
}

void MaterialEntityRenderer::doRender(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableMaterialEntityItem::render");
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;

    // Don't render if our parent is set or our material is null
    QUuid parentID;
    Transform renderTransform;
    graphics::MaterialPointer drawMaterial;
    Transform textureTransform;
    withReadLock([&] {
        parentID = _parentID;
        renderTransform = _renderTransform;
        drawMaterial = _drawMaterial;
        textureTransform.setTranslation(glm::vec3(_materialMappingPos, 0));
        textureTransform.setRotation(glm::vec3(0, 0, glm::radians(_materialMappingRot)));
        textureTransform.setScale(glm::vec3(_materialMappingScale, 1));
    });
    if (!parentID.isNull() || !drawMaterial) {
        return;
    }

    batch.setModelTransform(renderTransform);

    if (args->_renderMode != render::Args::RenderMode::SHADOW_RENDER_MODE) {
        drawMaterial->setTextureTransforms(textureTransform);

        // bind the material
        RenderPipelines::bindMaterial(drawMaterial, batch, args->_enableTexturing);
        args->_details._materialSwitches++;
    }

    // Draw!
    DependencyManager::get<GeometryCache>()->renderSphere(batch);

    args->_details._trianglesRendered += (int)DependencyManager::get<GeometryCache>()->getSphereTriangleCount();
}
