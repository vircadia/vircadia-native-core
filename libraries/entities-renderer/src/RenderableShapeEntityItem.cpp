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

#include <render-utils/simple_vert.h>
#include <render-utils/simple_frag.h>

//#define SHAPE_ENTITY_USE_FADE_EFFECT
#ifdef SHAPE_ENTITY_USE_FADE_EFFECT
#   include <FadeEffect.h>
#endif

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
#ifdef SHAPE_ENTITY_USE_FADE_EFFECT
    return getLocalRenderAlpha() < 1.0f;
#else
    if (_procedural && _procedural->isFading()) {
        float isFading = Interpolate::calculateFadeRatio(_procedural->getFadeStartTime()) < 1.0f;
        _procedural->setIsFading(isFading);
        return isFading;
    } else {
        return getLocalRenderAlpha() < 1.0f || EntityItem::isTransparent();
    }
#endif
}

namespace render {
    template <> const ItemKey payloadGetKey(const ShapePayload::Pointer& payload) {
        return payloadGetKey(std::static_pointer_cast<RenderableEntityItemProxy>(payload));
    }
    template <> const Item::Bound payloadGetBound(const ShapePayload::Pointer& payload) {
        return payloadGetBound(std::static_pointer_cast<RenderableEntityItemProxy>(payload));
    }
    template <> void payloadRender(const ShapePayload::Pointer& payload, RenderArgs* args) {
        payloadRender(std::static_pointer_cast<RenderableEntityItemProxy>(payload), args);
    }
    template <> uint32_t metaFetchMetaSubItems(const ShapePayload::Pointer& payload, ItemIDs& subItems) {
        return metaFetchMetaSubItems(std::static_pointer_cast<RenderableEntityItemProxy>(payload), subItems);
    }

    template <> const ShapeKey shapeGetShapeKey(const ShapePayload::Pointer& payload) {
        auto shapeKey = ShapeKey::Builder();
#ifdef SHAPE_ENTITY_USE_FADE_EFFECT
        shapeKey.withCustom(GeometryCache::CUSTOM_PIPELINE_NUMBER);
#endif
        auto entity = payload->_entity;
        if (entity->getLocalRenderAlpha() < 1.f) {
            shapeKey.withTranslucent();
        }
        return shapeKey.build();
    }
}

bool RenderableShapeEntityItem::addToScene(const EntityItemPointer& self, const render::ScenePointer& scene, render::Transaction& transaction) {
    _myItem = scene->allocateID();

    auto renderData = std::make_shared<ShapePayload>(self, _myItem);
    auto renderPayload = std::make_shared<ShapePayload::Payload>(renderData);

    render::Item::Status::Getters statusGetters;
    makeEntityItemStatusGetters(self, statusGetters);
    renderPayload->addStatusGetters(statusGetters);

    transaction.resetItem(_myItem, renderPayload);
#ifdef SHAPE_ENTITY_USE_FADE_EFFECT
    transaction.addTransitionToItem(_myItem, render::Transition::ELEMENT_ENTER_DOMAIN);
#endif
    return true;
}

void RenderableShapeEntityItem::computeShapeInfo(ShapeInfo& info) {

    // This will be called whenever DIRTY_SHAPE flag (set by dimension change, etc)
    // is set.

    const glm::vec3 entityDimensions = getDimensions();

    switch (_shape){
        case entity::Shape::Quad:
        case entity::Shape::Cube: {
            _collisionShapeType = SHAPE_TYPE_BOX;
        }
        break;
        case entity::Shape::Sphere: {

            float diameter = entityDimensions.x;
            const float MIN_DIAMETER = 0.001f;
            const float MIN_RELATIVE_SPHERICAL_ERROR = 0.001f;
            if (diameter > MIN_DIAMETER
                && fabsf(diameter - entityDimensions.y) / diameter < MIN_RELATIVE_SPHERICAL_ERROR
                && fabsf(diameter - entityDimensions.z) / diameter < MIN_RELATIVE_SPHERICAL_ERROR) {

                _collisionShapeType = SHAPE_TYPE_SPHERE;
            }
            else {
                _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
            }
        }
        break;
        case entity::Shape::Cylinder: {
            _collisionShapeType = SHAPE_TYPE_CYLINDER_Y;
            // TODO WL21389: determine if rotation is axis-aligned
            //const Transform::Quat & rot = _transform.getRotation();

            // TODO WL21389: some way to tell apart SHAPE_TYPE_CYLINDER_Y, _X, _Z based on rotation and
            //       hull ( or dimensions, need circular cross section)
            // Should allow for minor variance along axes?

        }
        break;
        case entity::Shape::Triangle:
        case entity::Shape::Hexagon:
        case entity::Shape::Octagon:
        case entity::Shape::Circle:
        case entity::Shape::Tetrahedron:
        case entity::Shape::Octahedron:
        case entity::Shape::Dodecahedron:
        case entity::Shape::Icosahedron:
        case entity::Shape::Cone: {
            //TODO WL21389: SHAPE_TYPE_SIMPLE_HULL and pointCollection (later)
            _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
        }
        break;
        case entity::Shape::Torus:
        {
            // Not in GeometryCache::buildShapes, unsupported.
            _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
            //TODO WL21389: SHAPE_TYPE_SIMPLE_HULL and pointCollection (later if desired support)
        }
        break;
        default:{
            _collisionShapeType = SHAPE_TYPE_ELLIPSOID;
        }
        break;
    }

    EntityItem::computeShapeInfo(info);
}

// This value specifes how the shape should be treated by physics calculations.
ShapeType RenderableShapeEntityItem::getShapeType() const {
    return _collisionShapeType;
}

void RenderableShapeEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableShapeEntityItem::render");
    //Q_ASSERT(getType() == EntityTypes::Shape);
    Q_ASSERT(args->_batch);
#ifndef SHAPE_ENTITY_USE_FADE_EFFECT
    checkFading();
#endif

    if (!_procedural) {
        _procedural.reset(new Procedural(getUserData()));
        _procedural->_vertexSource = simple_vert;
        _procedural->_fragmentSource = simple_frag;
        _procedural->_opaqueState->setCullMode(gpu::State::CULL_NONE);
        _procedural->_opaqueState->setDepthTest(true, true, gpu::LESS_EQUAL);
        PrepareStencil::testMaskDrawShape(*_procedural->_opaqueState);
        _procedural->_opaqueState->setBlendFunction(false,
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
#ifndef SHAPE_ENTITY_USE_FADE_EFFECT
        outColor.a *= _procedural->isFading() ? Interpolate::calculateFadeRatio(_procedural->getFadeStartTime()) : 1.0f;
#endif
        batch._glColor4f(outColor.r, outColor.g, outColor.b, outColor.a);
        if (render::ShapeKey(args->_globalShapeKey).isWireframe()) {
            DependencyManager::get<GeometryCache>()->renderWireShape(batch, MAPPING[_shape]);
        } else {
            DependencyManager::get<GeometryCache>()->renderShape(batch, MAPPING[_shape]);
        }
    } else {
        // FIXME, support instanced multi-shape rendering using multidraw indirect
        auto geometryCache = DependencyManager::get<GeometryCache>();
#ifdef SHAPE_ENTITY_USE_FADE_EFFECT
        auto shapeKey = render::ShapeKey(args->_itemShapeKey);
        
        assert(args->_shapePipeline != nullptr);

        if (shapeKey.isFaded()) {
            auto fadeEffect = DependencyManager::get<FadeEffect>();
            auto fadeCategory = fadeEffect->getLastCategory();
            auto fadeThreshold = fadeEffect->getLastThreshold();
            auto fadeNoiseOffset = fadeEffect->getLastNoiseOffset();
            auto fadeBaseOffset = fadeEffect->getLastBaseOffset();
            auto fadeBaseInvSize = fadeEffect->getLastBaseInvSize();

            if (shapeKey.isWireframe()) {
                geometryCache->renderWireFadeShapeInstance(args, batch, MAPPING[_shape], color, fadeCategory, fadeThreshold,
                    fadeNoiseOffset, fadeBaseOffset, fadeBaseInvSize, args->_shapePipeline);
            }
            else {
                geometryCache->renderSolidFadeShapeInstance(args, batch, MAPPING[_shape], color, fadeCategory, fadeThreshold,
                    fadeNoiseOffset, fadeBaseOffset, fadeBaseInvSize, args->_shapePipeline);
            }
        } else {
            if (shapeKey.isWireframe()) {
                geometryCache->renderWireShapeInstance(args, batch, MAPPING[_shape], color, args->_shapePipeline);
            }
            else {
                geometryCache->renderSolidShapeInstance(args, batch, MAPPING[_shape], color, args->_shapePipeline);
            }
        }
#else
        color.a *= _isFading ? Interpolate::calculateFadeRatio(_fadeStartTime) : 1.0f;
        auto pipeline = color.a < 1.0f ? geometryCache->getTransparentShapePipeline() : geometryCache->getOpaqueShapePipeline();
        if (render::ShapeKey(args->_globalShapeKey).isWireframe()) {
            geometryCache->renderWireShapeInstance(args, batch, MAPPING[_shape], color, pipeline);
        }
        else {
            geometryCache->renderSolidShapeInstance(args, batch, MAPPING[_shape], color, pipeline);
        }
#endif
    }

    static const auto triCount = DependencyManager::get<GeometryCache>()->getShapeTriangleCount(MAPPING[_shape]);
    args->_details._trianglesRendered += (int)triCount;
}
