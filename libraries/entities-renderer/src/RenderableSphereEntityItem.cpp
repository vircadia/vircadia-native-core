//
//  RenderableSphereEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableSphereEntityItem.h"

#include <glm/gtx/quaternion.hpp>

#include <gpu/Batch.h>

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <PerfStat.h>

#include "../render-utils/simple_vert.h"
#include "../render-utils/simple_frag.h"

// Sphere entities should fit inside a cube entity of the same size, so a sphere that has dimensions 1x1x1 
// is a half unit sphere.  However, the geometry cache renders a UNIT sphere, so we need to scale down.
static const float SPHERE_ENTITY_SCALE = 0.5f;


EntityItemPointer RenderableSphereEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity{ new RenderableSphereEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

void RenderableSphereEntityItem::setUserData(const QString& value) {
    if (value != getUserData()) {
        SphereEntityItem::setUserData(value);
        if (_procedural) {
            _procedural->parse(value);
        }
    }
}

void RenderableSphereEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableSphereEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Sphere);
    Q_ASSERT(args->_batch);

    if (!_procedural) {
        _procedural.reset(new Procedural(getUserData()));
        _procedural->_vertexSource = simple_vert;
        _procedural->_fragmentSource = simple_frag;
        _procedural->_state->setCullMode(gpu::State::CULL_NONE);
        _procedural->_state->setDepthTest(true, true, gpu::LESS_EQUAL);
        _procedural->_state->setBlendFunction(false,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
    }

    gpu::Batch& batch = *args->_batch;
    glm::vec4 sphereColor(toGlm(getXColor()), getLocalRenderAlpha());
    bool success;
    Transform modelTransform = getTransformToCenter(success);
    if (!success) {
        return;
    }
    modelTransform.postScale(SPHERE_ENTITY_SCALE);
    batch.setModelTransform(modelTransform); // use a transform with scale, rotation, registration point and translation
    if (_procedural->ready()) {
        _procedural->prepare(batch, getPosition(), getDimensions());
        auto color = _procedural->getColor(sphereColor);
        batch._glColor4f(color.r, color.g, color.b, color.a);
        DependencyManager::get<GeometryCache>()->renderSphere(batch);
    } else {
        DependencyManager::get<GeometryCache>()->renderSolidSphereInstance(batch, sphereColor);
    }
    static const auto triCount = DependencyManager::get<GeometryCache>()->getSphereTriangleCount();
    args->_details._trianglesRendered += (int)triCount;
}
