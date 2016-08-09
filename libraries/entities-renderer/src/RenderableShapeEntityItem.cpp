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

#include <render-utils/simple_vert.h>
#include <render-utils/simple_frag.h>

// Sphere entities should fit inside a cube entity of the same size, so a sphere that has dimensions 1x1x1 
// is a half unit sphere.  However, the geometry cache renders a UNIT sphere, so we need to scale down.
static const float SPHERE_ENTITY_SCALE = 0.5f;

static std::array<GeometryCache::Shape, entity::NUM_SHAPES> MAPPING { {
    GeometryCache::Triangle,
    GeometryCache::Quad,
    GeometryCache::Hexagon,
    GeometryCache::Octagon,
    GeometryCache::Circle,
    GeometryCache::Cube,
    GeometryCache::Sphere,
    GeometryCache::Tetrahedron,
    GeometryCache::Octahedron,
    GeometryCache::Dodecahedron,
    GeometryCache::Icosahedron,
    GeometryCache::Torus,
    GeometryCache::Cone,
    GeometryCache::Cylinder,
} };

RenderableShapeEntityItem::Pointer RenderableShapeEntityItem::baseFactory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    Pointer entity = std::make_shared<RenderableShapeEntityItem>(entityID);
    entity->setProperties(properties);
    return entity;
}

EntityItemPointer RenderableShapeEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return baseFactory(entityID, properties);
}

EntityItemPointer RenderableShapeEntityItem::boxFactory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    auto result = baseFactory(entityID, properties);
    result->setShape(entity::Cube);
    return result;
}

EntityItemPointer RenderableShapeEntityItem::sphereFactory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    auto result = baseFactory(entityID, properties);
    result->setShape(entity::Sphere);
    return result;
}

void RenderableShapeEntityItem::setUserData(const QString& value) {
    if (value != getUserData()) {
        ShapeEntityItem::setUserData(value);
        if (_procedural) {
            _procedural->parse(value);
        }
    }
}

bool RenderableShapeEntityItem::isTransparent() {
    if (_procedural && _procedural->isFading()) {
        float isFading = Interpolate::calculateFadeRatio(_procedural->getFadeStartTime()) < 1.0f;
        _procedural->setIsFading(isFading);
        return isFading;
    } else {
        return getLocalRenderAlpha() < 1.0f || EntityItem::isTransparent();
    }
}

void RenderableShapeEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableShapeEntityItem::render");
    //Q_ASSERT(getType() == EntityTypes::Shape);
    Q_ASSERT(args->_batch);
    checkFading();

    if (!_procedural) {
        _procedural.reset(new Procedural(getUserData()));
        _procedural->_vertexSource = simple_vert;
        _procedural->_fragmentSource = simple_frag;
        _procedural->_state->setCullMode(gpu::State::CULL_NONE);
        _procedural->_state->setDepthTest(true, true, gpu::LESS_EQUAL);
        _procedural->_state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
    }

    gpu::Batch& batch = *args->_batch;
    glm::vec4 color(toGlm(getXColor()), getLocalRenderAlpha());
    bool success;
    Transform modelTransform = getTransformToCenter(success);
    if (!success) {
        return;
    }
    if (_shape == entity::Sphere) {
        modelTransform.postScale(SPHERE_ENTITY_SCALE);
    }
    batch.setModelTransform(modelTransform); // use a transform with scale, rotation, registration point and translation
    if (_procedural->ready()) {
        _procedural->prepare(batch, getPosition(), getDimensions(), getOrientation());
        auto outColor = _procedural->getColor(color);
        outColor.a *= _procedural->isFading() ? Interpolate::calculateFadeRatio(_procedural->getFadeStartTime()) : 1.0f;
        batch._glColor4f(outColor.r, outColor.g, outColor.b, outColor.a);
        DependencyManager::get<GeometryCache>()->renderShape(batch, MAPPING[_shape]);
    } else {
        // FIXME, support instanced multi-shape rendering using multidraw indirect
        color.a *= _isFading ? Interpolate::calculateFadeRatio(_fadeStartTime) : 1.0f;
        auto geometryCache = DependencyManager::get<GeometryCache>();
        auto pipeline = color.a < 1.0f ? geometryCache->getTransparentShapePipeline() : geometryCache->getOpaqueShapePipeline();
        geometryCache->renderSolidShapeInstance(batch, MAPPING[_shape], color, pipeline);
    }

    static const auto triCount = DependencyManager::get<GeometryCache>()->getShapeTriangleCount(MAPPING[_shape]);
    args->_details._trianglesRendered += (int)triCount;
}
