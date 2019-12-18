//
//  Created by Bradley Austin Davis on 2016/05/09
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableShapeEntityItem.h"

#include <glm/gtx/quaternion.hpp>

#include <gpu/Batch.h>
#include <DependencyManager.h>
#include <GeometryCache.h>
#include <PerfStat.h>

#include "RenderPipelines.h"

using namespace render;
using namespace render::entities;

// Sphere entities should fit inside a cube entity of the same size, so a sphere that has dimensions 1x1x1 
// is a half unit sphere.  However, the geometry cache renders a UNIT sphere, so we need to scale down.
static const float SPHERE_ENTITY_SCALE = 0.5f;

ShapeEntityRenderer::ShapeEntityRenderer(const EntityItemPointer& entity) : Parent(entity) {
    addMaterial(graphics::MaterialLayer(_material, 0), "0");
}

bool ShapeEntityRenderer::needsRenderUpdate() const {
    if (resultWithReadLock<bool>([&] {
        auto mat = _materials.find("0");
        if (mat != _materials.end() && mat->second.top().material && mat->second.top().material->isProcedural() &&
            mat->second.top().material->isEnabled()) {
            auto procedural = std::static_pointer_cast<graphics::ProceduralMaterial>(mat->second.top().material);
            if (procedural->isFading()) {
                return true;
            }
        }

        if (mat != _materials.end() && mat->second.shouldUpdate()) {
            return true;
        }

        return false;
    })) {
        return true;
    }

    return Parent::needsRenderUpdate();
}

bool ShapeEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    if (_dimensions != entity->getScaledDimensions()) {
        return true;
    }

    if (_proceduralData != entity->getUserData()) {
        return true;
    }

    return false;
}

void ShapeEntityRenderer::doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) {
    withWriteLock([&] {
        _shape = entity->getShape();
        _pulseProperties = entity->getPulseProperties();
    });

    void* key = (void*)this;
    AbstractViewStateInterface::instance()->pushPostUpdateLambda(key, [this] () {
        withWriteLock([&] {
            auto entity = getEntity();
            _position = entity->getWorldPosition();
            _dimensions = entity->getUnscaledDimensions(); // get unscaled to avoid scaling twice
            _orientation = entity->getWorldOrientation();
            updateModelTransformAndBound();
            _renderTransform = getModelTransform(); // contains parent scale, if this entity scales with its parent
            if (_shape == entity::Sphere) {
                _renderTransform.postScale(SPHERE_ENTITY_SCALE);
            }

            _renderTransform.postScale(_dimensions);
        });;
    });
}

void ShapeEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    withReadLock([&] {
        auto mat = _materials.find("0");
        if (mat != _materials.end() && mat->second.top().material && mat->second.top().material->isProcedural() && mat->second.top().material->isEnabled()) {
            auto procedural = std::static_pointer_cast<graphics::ProceduralMaterial>(mat->second.top().material);
            if (procedural->isFading()) {
                procedural->setIsFading(Interpolate::calculateFadeRatio(procedural->getFadeStartTime()) < 1.0f);
            }
        }
    });

    withWriteLock([&] {
        bool materialChanged = false;
        glm::vec3 color = toGlm(entity->getColor());
        if (_color != color) {
            _color = color;
            _material->setAlbedo(color);
            materialChanged = true;
        }

        float alpha = entity->getAlpha();
        if (_alpha != alpha) {
            _alpha = alpha;
            _material->setOpacity(alpha);
            materialChanged = true;
        }

        auto userData = entity->getUserData();
        if (_proceduralData != userData) {
            _proceduralData = userData;
            _material->setProceduralData(_proceduralData);
            materialChanged = true;
        }

        if (materialChanged) {
            auto materials = _materials.find("0");
            if (materials != _materials.end()) {
                materials->second.setNeedsUpdate(true);
            }
        }
    });
}

bool ShapeEntityRenderer::isTransparent() const {
    if (_pulseProperties.getAlphaMode() != PulseMode::NONE) {
        return true;
    }

    auto mat = _materials.find("0");
    if (mat != _materials.end() && mat->second.top().material) {
        if (mat->second.top().material->isProcedural() && mat->second.top().material->isEnabled()) {
            auto procedural = std::static_pointer_cast<graphics::ProceduralMaterial>(mat->second.top().material);
            if (procedural->isFading()) {
                return true;
            }
        }

        if (mat->second.getMaterialKey().isTranslucent()) {
            return true;
        }
    }

    return Parent::isTransparent();
}

ShapeEntityRenderer::Pipeline ShapeEntityRenderer::getPipelineType(const graphics::MultiMaterial& materials) const {
    if (materials.top().material && materials.top().material->isProcedural() && materials.top().material->isReady()) {
        return Pipeline::PROCEDURAL;
    }

    graphics::MaterialKey drawMaterialKey = materials.getMaterialKey();
    if (drawMaterialKey.isEmissive() || drawMaterialKey.isUnlit() || drawMaterialKey.isMetallic() || drawMaterialKey.isScattering()) {
        return Pipeline::MATERIAL;
    }

    // If the material is using any map, we need to use a material ShapeKey
    for (int i = 0; i < graphics::Material::MapChannel::NUM_MAP_CHANNELS; i++) {
        if (drawMaterialKey.isMapChannel(graphics::Material::MapChannel(i))) {
            return Pipeline::MATERIAL;
        }
    }
    return Pipeline::SIMPLE;
}

ShapeKey ShapeEntityRenderer::getShapeKey() {
    ShapeKey::Builder builder;
    auto mat = _materials.find("0");
    if (mat != _materials.end() && mat->second.shouldUpdate()) {
        RenderPipelines::updateMultiMaterial(mat->second);
    }

    if (isTransparent()) {
        builder.withTranslucent();
    }

    if (_primitiveMode == PrimitiveMode::LINES) {
        builder.withWireframe();
    }

    auto pipelineType = getPipelineType(mat->second);
    if (pipelineType == Pipeline::MATERIAL) {
        builder.withMaterial();

        graphics::MaterialKey drawMaterialKey = mat->second.getMaterialKey();
        if (drawMaterialKey.isNormalMap()) {
            builder.withTangents();
        }
        if (drawMaterialKey.isLightMap()) {
            builder.withLightMap();
        }
        if (drawMaterialKey.isUnlit()) {
            builder.withUnlit();
        }
        builder.withCullFaceMode(mat->second.getCullFaceMode());
    } else if (pipelineType == Pipeline::PROCEDURAL) {
        builder.withOwnPipeline();
    }

    return builder.build();
}

Item::Bound ShapeEntityRenderer::getBound() {
    auto mat = _materials.find("0");
    if (mat != _materials.end() && mat->second.top().material && mat->second.top().material->isProcedural() &&
        mat->second.top().material->isReady()) {
        auto procedural = std::static_pointer_cast<graphics::ProceduralMaterial>(mat->second.top().material);
        if (procedural->hasVertexShader() && procedural->hasBoundOperator()) {
           return procedural->getBound();
        }
    }
    return Parent::getBound();
}

void ShapeEntityRenderer::doRender(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableShapeEntityItem::render");
    Q_ASSERT(args->_batch);

    gpu::Batch& batch = *args->_batch;

    graphics::MultiMaterial materials;
    auto geometryCache = DependencyManager::get<GeometryCache>();
    GeometryCache::Shape geometryShape;
    PrimitiveMode primitiveMode;
    RenderLayer renderLayer;
    glm::vec4 outColor;
    Pipeline pipelineType;
    withReadLock([&] {
        geometryShape = geometryCache->getShapeForEntityShape(_shape);
        primitiveMode = _primitiveMode;
        renderLayer = _renderLayer;
        batch.setModelTransform(_renderTransform); // use a transform with scale, rotation, registration point and translation
        materials = _materials["0"];
        pipelineType = getPipelineType(materials);
        auto& schema = materials.getSchemaBuffer().get<graphics::MultiMaterial::Schema>();
        outColor = glm::vec4(ColorUtils::tosRGBVec3(schema._albedo), schema._opacity);
        outColor = EntityRenderer::calculatePulseColor(outColor, _pulseProperties, _created);
    });

    if (outColor.a == 0.0f) {
        return;
    }

    if (pipelineType == Pipeline::PROCEDURAL) {
        auto procedural = std::static_pointer_cast<graphics::ProceduralMaterial>(materials.top().material);
        outColor = procedural->getColor(outColor);
        outColor.a *= procedural->isFading() ? Interpolate::calculateFadeRatio(procedural->getFadeStartTime()) : 1.0f;
        procedural->prepare(batch, _position, _dimensions, _orientation, _created, ProceduralProgramKey(outColor.a < 1.0f));

        if (render::ShapeKey(args->_globalShapeKey).isWireframe() || primitiveMode == PrimitiveMode::LINES) {
            geometryCache->renderWireShape(batch, geometryShape, outColor);
        } else {
            geometryCache->renderShape(batch, geometryShape, outColor);
        }
    } else if (pipelineType == Pipeline::SIMPLE) {
        // FIXME, support instanced multi-shape rendering using multidraw indirect
        outColor.a *= _isFading ? Interpolate::calculateFadeRatio(_fadeStartTime) : 1.0f;
        render::ShapePipelinePointer pipeline = geometryCache->getShapePipelinePointer(outColor.a < 1.0f, false,
            renderLayer != RenderLayer::WORLD || args->_renderMethod == Args::RenderMethod::FORWARD, materials.top().material->getCullFaceMode());
        if (render::ShapeKey(args->_globalShapeKey).isWireframe() || primitiveMode == PrimitiveMode::LINES) {
            geometryCache->renderWireShapeInstance(args, batch, geometryShape, outColor, pipeline);
        } else {
            geometryCache->renderSolidShapeInstance(args, batch, geometryShape, outColor, pipeline);
        }
    } else {
        if (RenderPipelines::bindMaterials(materials, batch, args->_renderMode, args->_enableTexturing)) {
            args->_details._materialSwitches++;
        }

        geometryCache->renderShape(batch, geometryShape);
    }

    const auto triCount = geometryCache->getShapeTriangleCount(geometryShape);
    args->_details._trianglesRendered += (int)triCount;
}

scriptable::ScriptableModelBase ShapeEntityRenderer::getScriptableModel()  {
    scriptable::ScriptableModelBase result;
    auto geometryCache = DependencyManager::get<GeometryCache>();
    auto geometryShape = geometryCache->getShapeForEntityShape(_shape);
    glm::vec3 vertexColor;
    {
        std::lock_guard<std::mutex> lock(_materialsLock);
        result.appendMaterials(_materials);
        auto materials = _materials.find("0");
        if (materials != _materials.end()) {
            vertexColor = ColorUtils::tosRGBVec3(materials->second.getSchemaBuffer().get<graphics::MultiMaterial::Schema>()._albedo);
        }
    }
    if (auto mesh = geometryCache->meshFromShape(geometryShape, vertexColor)) {
        result.objectID = getEntity()->getID();
        result.append(mesh);
    }
    return result;
}
