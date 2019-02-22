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
#include <StencilMaskPass.h>
#include <GeometryCache.h>
#include <PerfStat.h>
#include <shaders/Shaders.h>

#include "RenderPipelines.h"

//#define SHAPE_ENTITY_USE_FADE_EFFECT
#ifdef SHAPE_ENTITY_USE_FADE_EFFECT
#include <FadeEffect.h>
#endif
using namespace render;
using namespace render::entities;

// Sphere entities should fit inside a cube entity of the same size, so a sphere that has dimensions 1x1x1 
// is a half unit sphere.  However, the geometry cache renders a UNIT sphere, so we need to scale down.
static const float SPHERE_ENTITY_SCALE = 0.5f;

#if defined(USE_GLES)
static bool DISABLE_DEFERRED = true;
#else
static const QString RENDER_FORWARD{ "HIFI_RENDER_FORWARD" };
static bool DISABLE_DEFERRED = QProcessEnvironment::systemEnvironment().contains(RENDER_FORWARD);
#endif

static_assert(shader::render_utils::program::simple != 0, "Validate simple program exists");
static_assert(shader::render_utils::program::simple_transparent != 0, "Validate simple transparent program exists");

ShapeEntityRenderer::ShapeEntityRenderer(const EntityItemPointer& entity) : Parent(entity) {
    _procedural._vertexSource = gpu::Shader::getVertexShaderSource(shader::render_utils::vertex::simple);
    // FIXME: Setup proper uniform slots and use correct pipelines for forward rendering
    _procedural._opaqueFragmentSource = gpu::Shader::Source::get(shader::render_utils::fragment::simple);
    _procedural._transparentFragmentSource = gpu::Shader::Source::get(shader::render_utils::fragment::simple_transparent);

    // TODO: move into Procedural.cpp
    PrepareStencil::testMaskDrawShape(*_procedural._opaqueState);
    PrepareStencil::testMask(*_procedural._transparentState);

    addMaterial(graphics::MaterialLayer(_material, 0), "0");
}

bool ShapeEntityRenderer::needsRenderUpdate() const {
    if (resultWithReadLock<bool>([&] {
        if (_procedural.isEnabled() && _procedural.isFading()) {
            return true;
        }

        auto mat = _materials.find("0");
        if (mat != _materials.end() && (mat->second.needsUpdate() || mat->second.areTexturesLoading())) {
            return true;
        }

        return false;
    })) {
        return true;
    }

    return Parent::needsRenderUpdate();
}

bool ShapeEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    if (_lastUserData != entity->getUserData()) {
        return true;
    }

    if (_color != entity->getColor()) {
        return true;
    }
    if (_alpha != entity->getAlpha()) {
        return true;
    }

    if (_shape != entity->getShape()) {
        return true;
    }

    if (_dimensions != entity->getScaledDimensions()) {
        return true;
    }

    if (_pulseProperties != entity->getPulseProperties()) {
        return true;
    }

    return false;
}

void ShapeEntityRenderer::doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) {
    withWriteLock([&] {
        auto userData = entity->getUserData();
        if (_lastUserData != userData) {
            _lastUserData = userData;
            _procedural.setProceduralData(ProceduralData::parse(_lastUserData));
        }

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
        if (_procedural.isEnabled() && _procedural.isFading()) {
            float isFading = Interpolate::calculateFadeRatio(_procedural.getFadeStartTime()) < 1.0f;
            _procedural.setIsFading(isFading);
        }
    });

    glm::u8vec3 color = entity->getColor();
    float alpha = entity->getAlpha();
    if (_color != color || _alpha != alpha) {
        _color = color;
        _alpha = alpha;
        _material->setAlbedo(toGlm(_color));
        _material->setOpacity(_alpha);

        auto materials = _materials.find("0");
        if (materials != _materials.end()) {
            materials->second.setNeedsUpdate(true);
        }
    }
}

bool ShapeEntityRenderer::isTransparent() const {
    if (_pulseProperties.getAlphaMode() != PulseMode::NONE) {
        return true;
    }

    if (_procedural.isEnabled() && _procedural.isFading()) {
        return Interpolate::calculateFadeRatio(_procedural.getFadeStartTime()) < 1.0f;
    }

    auto mat = _materials.find("0");
    if (mat != _materials.end()) {
        if (mat->second.getMaterialKey().isTranslucent()) {
            return true;
        }
    }

    return Parent::isTransparent();
}

bool ShapeEntityRenderer::useMaterialPipeline(const graphics::MultiMaterial& materials) const {
    bool proceduralReady = resultWithReadLock<bool>([&] {
        return _procedural.isReady();
    });
    if (proceduralReady) {
        return false;
    }

    graphics::MaterialKey drawMaterialKey = materials.getMaterialKey();
    if (drawMaterialKey.isEmissive() || drawMaterialKey.isUnlit() || drawMaterialKey.isMetallic() || drawMaterialKey.isScattering()) {
        return true;
    }

    // If the material is using any map, we need to use a material ShapeKey
    for (int i = 0; i < graphics::Material::MapChannel::NUM_MAP_CHANNELS; i++) {
        if (drawMaterialKey.isMapChannel(graphics::Material::MapChannel(i))) {
            return true;
        }
    }
    return false;
}

ShapeKey ShapeEntityRenderer::getShapeKey() {
    auto mat = _materials.find("0");
    if (mat != _materials.end() && (mat->second.needsUpdate() || mat->second.areTexturesLoading())) {
        RenderPipelines::updateMultiMaterial(mat->second);
    }

    if (mat != _materials.end() && useMaterialPipeline(mat->second)) {
        graphics::MaterialKey drawMaterialKey = mat->second.getMaterialKey();

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
    } else {
        ShapeKey::Builder builder;
        bool proceduralReady = resultWithReadLock<bool>([&] {
            return _procedural.isReady();
        });
        if (proceduralReady) {
            builder.withOwnPipeline();
        }
        if (isTransparent()) {
            builder.withTranslucent();
        }

        if (_primitiveMode == PrimitiveMode::LINES) {
            builder.withWireframe();
        }
        return builder.build();
    }
}

void ShapeEntityRenderer::doRender(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableShapeEntityItem::render");
    Q_ASSERT(args->_batch);

    gpu::Batch& batch = *args->_batch;

    graphics::MultiMaterial materials;
    auto geometryCache = DependencyManager::get<GeometryCache>();
    GeometryCache::Shape geometryShape;
    bool proceduralRender = false;
    glm::vec4 outColor;
    withReadLock([&] {
        geometryShape = geometryCache->getShapeForEntityShape(_shape);
        batch.setModelTransform(_renderTransform); // use a transform with scale, rotation, registration point and translation
        materials = _materials["0"];
        auto& schema = materials.getSchemaBuffer().get<graphics::MultiMaterial::Schema>();
        outColor = glm::vec4(ColorUtils::tosRGBVec3(schema._albedo), schema._opacity);
        outColor = EntityRenderer::calculatePulseColor(outColor, _pulseProperties, _created);
        if (_procedural.isReady()) {
            outColor = _procedural.getColor(outColor);
            outColor.a *= _procedural.isFading() ? Interpolate::calculateFadeRatio(_procedural.getFadeStartTime()) : 1.0f;
            _procedural.prepare(batch, _position, _dimensions, _orientation, ProceduralProgramKey(outColor.a < 1.0f));
            proceduralRender = true;
        }
    });

    if (proceduralRender) {
        if (render::ShapeKey(args->_globalShapeKey).isWireframe()) {
            geometryCache->renderWireShape(batch, geometryShape, outColor);
        } else {
            geometryCache->renderShape(batch, geometryShape, outColor);
        }
    } else if (!useMaterialPipeline(materials)) {
        // FIXME, support instanced multi-shape rendering using multidraw indirect
        outColor.a *= _isFading ? Interpolate::calculateFadeRatio(_fadeStartTime) : 1.0f;
        render::ShapePipelinePointer pipeline;
        if (_renderLayer == RenderLayer::WORLD && !DISABLE_DEFERRED) {
            pipeline = outColor.a < 1.0f ? geometryCache->getTransparentShapePipeline() : geometryCache->getOpaqueShapePipeline();
        } else {
            pipeline = outColor.a < 1.0f ? geometryCache->getForwardTransparentShapePipeline() : geometryCache->getForwardOpaqueShapePipeline();
        }
        if (render::ShapeKey(args->_globalShapeKey).isWireframe() || _primitiveMode == PrimitiveMode::LINES) {
            geometryCache->renderWireShapeInstance(args, batch, geometryShape, outColor, pipeline);
        } else {
            geometryCache->renderSolidShapeInstance(args, batch, geometryShape, outColor, pipeline);
        }
    } else {
        if (args->_renderMode != render::Args::RenderMode::SHADOW_RENDER_MODE) {
            RenderPipelines::bindMaterials(materials, batch, args->_enableTexturing);
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
