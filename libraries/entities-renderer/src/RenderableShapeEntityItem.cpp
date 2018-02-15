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

#include "render-utils/simple_vert.h"
#include "render-utils/simple_frag.h"

//#define SHAPE_ENTITY_USE_FADE_EFFECT
#ifdef SHAPE_ENTITY_USE_FADE_EFFECT
#include <FadeEffect.h>
#endif
using namespace render;
using namespace render::entities;

// Sphere entities should fit inside a cube entity of the same size, so a sphere that has dimensions 1x1x1 
// is a half unit sphere.  However, the geometry cache renders a UNIT sphere, so we need to scale down.
static const float SPHERE_ENTITY_SCALE = 0.5f;


ShapeEntityRenderer::ShapeEntityRenderer(const EntityItemPointer& entity) : Parent(entity) {
    _procedural._vertexSource = simple_vert::getSource();
    _procedural._fragmentSource = simple_frag::getSource();
    _procedural._opaqueState->setCullMode(gpu::State::CULL_NONE);
    _procedural._opaqueState->setDepthTest(true, true, gpu::LESS_EQUAL);
    PrepareStencil::testMaskDrawShape(*_procedural._opaqueState);
    _procedural._opaqueState->setBlendFunction(false,
        gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
        gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
}

bool ShapeEntityRenderer::needsRenderUpdate() const {
    if (_procedural.isEnabled() && _procedural.isFading()) {
        return true;
    }

    return Parent::needsRenderUpdate();
}

bool ShapeEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    if (_lastUserData != entity->getUserData()) {
        return true;
    }
    glm::vec4 newColor(toGlm(entity->getXColor()), entity->getLocalRenderAlpha());
    if (newColor != _color) {
        return true;
    }

    if (_shape != entity->getShape()) {
        return true;
    }

    if (_dimensions != entity->getScaledDimensions()) {
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

        _color = vec4(toGlm(entity->getXColor()), entity->getLocalRenderAlpha());

        _shape = entity->getShape();
        _position = entity->getWorldPosition();
        _dimensions = entity->getScaledDimensions();
        _orientation = entity->getWorldOrientation();
        _renderTransform = getModelTransform();

        if (_shape == entity::Sphere) {
            _renderTransform.postScale(SPHERE_ENTITY_SCALE);
        }

        _renderTransform.postScale(_dimensions);
    });
}

void ShapeEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    withReadLock([&] {
        if (_procedural.isEnabled() && _procedural.isFading()) {
            float isFading = Interpolate::calculateFadeRatio(_procedural.getFadeStartTime()) < 1.0f;
            _procedural.setIsFading(isFading);
        }
    });
}

bool ShapeEntityRenderer::isTransparent() const {
    if (_procedural.isEnabled() && _procedural.isFading()) {
        return Interpolate::calculateFadeRatio(_procedural.getFadeStartTime()) < 1.0f;
    }
    
    //        return _entity->getLocalRenderAlpha() < 1.0f || Parent::isTransparent();
    return Parent::isTransparent();
}



void ShapeEntityRenderer::doRender(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableShapeEntityItem::render");
    Q_ASSERT(args->_batch);

    gpu::Batch& batch = *args->_batch;

    auto geometryCache = DependencyManager::get<GeometryCache>();
    GeometryCache::Shape geometryShape;
    bool proceduralRender = false;
    glm::vec4 outColor;
    withReadLock([&] {
        geometryShape = geometryCache->getShapeForEntityShape(_shape);
        batch.setModelTransform(_renderTransform); // use a transform with scale, rotation, registration point and translation
        outColor = _color;
        if (_procedural.isReady()) {
            _procedural.prepare(batch, _position, _dimensions, _orientation);
            outColor = _procedural.getColor(_color);
            outColor.a *= _procedural.isFading() ? Interpolate::calculateFadeRatio(_procedural.getFadeStartTime()) : 1.0f;
            proceduralRender = true;
        }
    });

    if (proceduralRender) {
        if (render::ShapeKey(args->_globalShapeKey).isWireframe()) {
            geometryCache->renderWireShape(batch, geometryShape, outColor);
        } else {
            geometryCache->renderShape(batch, geometryShape, outColor);
        }
    } else {
        // FIXME, support instanced multi-shape rendering using multidraw indirect
        outColor.a *= _isFading ? Interpolate::calculateFadeRatio(_fadeStartTime) : 1.0f;
        auto pipeline = outColor.a < 1.0f ? geometryCache->getTransparentShapePipeline() : geometryCache->getOpaqueShapePipeline();
        if (render::ShapeKey(args->_globalShapeKey).isWireframe()) {
            geometryCache->renderWireShapeInstance(args, batch, geometryShape, outColor, pipeline);
        } else {
            geometryCache->renderSolidShapeInstance(args, batch, geometryShape, outColor, pipeline);
        }
    }

    const auto triCount = geometryCache->getShapeTriangleCount(geometryShape);
    args->_details._trianglesRendered += (int)triCount;
}
